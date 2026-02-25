#include "door_core.h"     //门禁核心类头文件
#include "log_util.h"      // 日志工具（postLog/g_log_queue）
#include "gpio_control.h"  // GPIO硬件控制（开门/报警）
#include "config.h"        // 系统配置参数（常量定义）
#include <opencv2/face.hpp>// OpenCV人脸识别模块（LBPH算法）
#include <iostream>        // 标准输入输出（日志打印）
#include <atomic>          // 原子变量（人脸框/识别结果）
#include <opencv2/opencv.hpp>
#include <thread>
#include <mutex>
#include <chrono>          //用于线程延迟

using namespace cv;
using namespace cv::face;
using namespace std;

/**
 * @brief 全局LBPH人脸识别器对象
 * @details 全局唯一，在DoorCore构造函数中初始化并加载训练好的模型
 *          LBPH：局部二值模式直方图，适合低成本人脸识别场景
 */
Ptr<LBPHFaceRecognizer> g_lbph;

atomic<bool> is_running_(true); //用于控制所有线程的循环退出，atomic保证多线程读写安全
atomic<Rect> g_face_rect;       // 人脸矩形框,存储检测到的人脸区域坐标
atomic<bool> g_recognize_success(false); // 识别结果（true=成功，false=失败）

/**
 * @brief 构造函数：初始化门禁系统核心资源
 * @details 1. 初始化GPIO硬件（继电器/蜂鸣器）
 *          2. 创建LBPH人脸识别器实例
 *          3. 加载预训练的人脸识别模型（MODEL_PATH）
 *          4. 输出初始化成功日志
 */
DoorCore::DoorCore() {
    
    // 设置DISPLAY环境变量：指定X11显示设备
    setenv("DISPLAY", ":0", 1);
    // 禁用GStreamer：避免OpenCV视频采集兼容问题
    setenv("OPENCV_VIDEOIO_DISABLE_GSTREAMER", "1", 1);
    
    gpioInit();// 初始化GPIO硬件（继电器/蜂鸣器）
    g_lbph = LBPHFaceRecognizer::create();// 创建LBPH人脸识别器（默认参数：半径1，邻域8，网格8x8，阈值默认）
    g_lbph->read(MODEL_PATH);             // 加载训练好的模型文件,MODEL_PATH从config.h引入
    postLog("[系统] 模型加载成功，门禁已就绪");// 记录初始化日志，告知用户系统就绪

    //初始化显示窗口
    namedWindow("人脸识别门禁系统", WINDOW_NORMAL);
    resizeWindow("人脸识别门禁系统", 800, 480); // 适配树莓派屏幕
    g_face_rect = Rect();        // 初始化人脸框为空
    g_recognize_success = false; // 初始化识别结果
}

/**
 * @brief 析构函数：停止系统，释放资源
 * @details 1. 设置运行状态为false，终止所有线程循环
 *          2. 停止所有线程安全队列（唤醒阻塞的pop操作）
 *          3. 等待所有业务线程结束，避免线程残留/资源泄漏
 */
DoorCore::~DoorCore() {
    is_running_ = false;// 设置原子变量为false，所有线程的while循环会退出
    frame_queue_.stop();// 停止帧队列
    face_queue_.stop(); //人脸队列
    g_log_queue.stop(); //日志队列,唤醒阻塞的pop线程

    // 等待各线程结束（需检查joinable，避免重复join崩溃）
    if (cap_thread_.joinable()) cap_thread_.join();      // 采集线程
    if (detect_thread_.joinable()) detect_thread_.join();// 检测线程
    if (rec_thread_.joinable()) rec_thread_.join();      // 识别线程
    if (log_thread_.joinable()) log_thread_.join();      // 日志线程
 
    destroyAllWindows();//销毁窗口
}

/**
 * @brief 启动门禁系统主函数
 * @details 核心流程：
 *          1. 设置系统运行状态为true
 *          2. 启动4个后台业务线程（采集、检测、识别、日志）
 *          3. 主线程进入显示循环：
 *             - 从帧队列取最新帧
 *             - 绘制人脸框和识别结果提示文字
 *             - 显示画面，监听ESC键退出
 *          4. 退出显示循环后，等待所有线程结束
 *          5. 销毁显示窗口，释放资源
 * @note 主线程负责画面显示和用户交互，后台线程负责数据处理
 */
void DoorCore::startSystem() {
    is_running_ = true;// 重置系统运行状态

    // 启动后台线程
    cap_thread_    = thread(&DoorCore::captureThread, this);
    detect_thread_ = thread(&DoorCore::detectThread, this);
    rec_thread_    = thread(&DoorCore::recognizeThread, this);
    log_thread_    = thread(&DoorCore::logThread, this);

    // 主线程： 初始化显示窗口
    namedWindow("人脸识别门禁系统", WINDOW_NORMAL);
    resizeWindow("人脸识别门禁系统", 640, 480);

    // 存储最新帧（用于显示）
    Mat latest_frame;
    while (is_running_) {
         // 非阻塞从帧队列取最新帧（超时则保留上一帧，避免画面卡顿）
        if (frame_queue_.pop(latest_frame) ){
            // 成功取到帧，继续显示
        } else {
        // 超时，没有新帧，保持上一帧显示
        }

        // 仅当有有效帧时才绘制和显示
        if (!latest_frame.empty()) {
            // 克隆帧（避免原帧被其他线程修改）
            Mat show_frame = latest_frame.clone();
            // 读取人脸框
            Rect face_rect = g_face_rect;
            // 检测到人脸时绘制人脸框和提示文字
            if (!face_rect.empty()) {
                 // 识别成功→绿色，识别失败→红色
                Scalar color = g_recognize_success ? Scalar(0,255,0) : Scalar(0,0,255);
                // 绘制人脸矩形框（线宽2）
                rectangle(show_frame, face_rect, color, 2);
                // 提示文字内容
                string text = g_recognize_success ? "识别成功 - 开门" : "识别失败 - 报警";
                // 绘制提示文字（位置：人脸框上方10像素，字体大小0.8，线宽2）
                putText(show_frame, text, Point(face_rect.x, face_rect.y - 10),  
                FONT_HERSHEY_SIMPLEX, 0.8, color, 2);
            }
            // 显示处理后的画面
            imshow("人脸识别门禁系统", show_frame);
        }

            if (waitKey(1) == 27) { // 按ESC退出
                is_running_ = false;// 设置运行状态为false，终止所有线程
                break;
            }
        }

    // 等待所有线程结束
    if (cap_thread_.joinable()) cap_thread_.join();
    if (detect_thread_.joinable()) detect_thread_.join();
    if (rec_thread_.joinable()) rec_thread_.join();
    if (log_thread_.joinable()) log_thread_.join();

    destroyAllWindows();// 销毁显示窗口，释放资源
}

/**
 * @brief 摄像头采集线程：采集视频帧并写入帧队列
 * @details 核心流程：
 *          1. 延迟2秒启动（等待系统初始化完成）
 *          2. 打开V4L2摄像头（设备0，树莓派默认摄像头）
 *          3. 配置摄像头参数（MJPG格式、640x480分辨率、25FPS）
 *          4. 循环采集帧，写入帧队列（线程安全）
 *          5. 系统停止时退出循环，释放摄像头资源
 */
void DoorCore::captureThread() {
    // 延迟2秒启动：等待构造函数初始化完成，避免摄像头抢占资源
    this_thread::sleep_for(chrono::seconds(2));
    // 打开摄像头
    VideoCapture cap(0, CAP_V4L2);
    if (!cap.isOpened()) {
        postLog("[错误] 摄像头打开失败");
        return;
    }

    // 强制使用 MJPG 格式
    cap.set(CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    // 设置采集分辨率：640x480（平衡清晰度和性能）
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    // 设置帧率：25FPS
    cap.set(CAP_PROP_FPS, 25);

    postLog("[线程] 采集线程启动(MJPG格式)");// 记录线程启动日志

    // 存储采集到的帧
    Mat frame;
    while (is_running_) {
        // 采集一帧并检查有效性
        if (cap.read(frame) && !frame.empty()) {
            // 将帧克隆后写入帧队列（避免原帧被覆盖）
            frame_queue_.push(frame.clone());
        } else {
             // 采集失败时短暂休眠（避免空循环占用CPU）
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
}

/**
 * @brief 人脸检测线程：从帧队列取帧，检测人脸并存入人脸队列
 * @details 核心流程：
 *          1. 加载Haar级联分类器（预训练人脸检测模型）
 *          2. 循环从帧队列取帧，预处理（转灰度图+直方图均衡化）
 *          3. 检测人脸，取第一个人脸区域存入人脸队列
 *          4. 更新全局人脸框原子变量（供主线程绘制）
 *          5. 系统停止时退出循环
 * @note 预处理步骤（灰度+均衡化）大幅提升低光照下的检测准确率
 */
void DoorCore::detectThread() {
    postLog("[线程] 检测线程启动");
    // 创建Haar级联分类器（用于人脸检测）
    CascadeClassifier face_cascade;
    // 加载预训练的人脸检测器模型（HAAR_PATH从config.h引入）
    face_cascade.load(HAAR_PATH);

    Mat frame, gray;   // 原始帧、灰度帧
    vector<Rect> faces;// 存储检测到的人脸矩形区域

    // 循环检测，直到系统停止
    while (is_running_) {
        // 从帧队列阻塞取帧（队列空则等待，stop则返回false）
        if (!frame_queue_.pop(frame)) continue;
        // 转为灰度图（人脸检测需灰度图，减少计算量）
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        // 直方图均衡化（增强对比度，提升检测准确率）
        equalizeHist(gray, gray);
        // 检测人脸：参数（灰度图，人脸区域，缩放因子，邻域数，过滤规则，最小人脸尺寸）
        face_cascade.detectMultiScale(gray, faces, 1.1, 4, 0, Size(60, 60));
        // 检测到人脸时，取第一个人脸区域存入人脸队列
        if (!faces.empty()) {
            face_queue_.push(gray(faces[0]));// gray(faces[0])：截取人脸区域
            g_face_rect = faces[0];//记录人脸矩形框坐标
        }else{
             g_face_rect = Rect();//无人脸时清空矩形框
        }
    }
}

/**
 * @brief 人脸识别线程：从人脸队列取人脸，识别并控制硬件
 * @details 核心流程：
 *          1. 循环从人脸队列取人脸图像
 *          2. 调用LBPH模型预测（输出标签+置信度）
 *          3. 判断识别结果：
 *             - 成功（标签有效+置信度<阈值）：记录日志→开门→更新识别结果
 *             - 失败（标签无效/置信度≥阈值）：记录日志→报警→更新识别结果
 *          4. 系统停止时退出循环
 * @note LBPH置信度越小表示匹配度越高，阈值从config.h的RECOGNIZE_THRESHOLD获取
 */
void DoorCore::recognizeThread() {
    postLog("[线程] 识别线程启动");
    Mat face;          // 存储人脸图像
    int label = -1;    // 识别标签（-1表示未识别）
    double conf = 0.0; // 置信度（距离值，越小越相似）

    // 循环识别，直到系统停止
    while (is_running_) {
        // 从人脸队列阻塞取人脸（队列空则等待，stop则返回false）
        if (!face_queue_.pop(face)) continue;
        // LBPH模型预测：输入人脸，输出标签+置信度
        g_lbph->predict(face, label, conf);
        // 识别成功：标签有效 且 置信度<阈值（RECOGNIZE_THRESHOLD=50）
        if (label != -1 && conf < RECOGNIZE_THRESHOLD) {
            postLog("[成功] ID=" + to_string(label) + " 置信度=" + to_string((int)conf));
            openDoorDelay();// 调用GPIO控制函数，开门2秒
            g_recognize_success = true;//记录识别成功
        } else {
            postLog("[失败] 未知人脸，置信度=" + to_string((int)conf));
            alarmBeep(); // 调用GPIO控制函数，蜂鸣器报警0.5秒
            g_recognize_success = false;//记录识别失败
        }
    }
}

/**
 * @brief 日志线程：异步输出日志到控制台
 * @details 核心流程：
 *          1. 循环从全局日志队列取日志消息
 *          2. 将消息打印到控制台（std::cout）
 *          3. 系统停止时退出循环
 */
void DoorCore::logThread() {
    string msg;// 存储日志消息
    // 循环处理日志，直到系统停止
    while (is_running_) {
        // 从日志队列取消息，成功则打印到控制台s
        if (g_log_queue.pop(msg)) {
            cout << msg << endl;
        }
    }
}




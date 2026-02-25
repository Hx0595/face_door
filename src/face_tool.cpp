/**
 * @file face_tool.cpp
 * @brief 人脸工具类实现（采集+训练）
 * @details 1. collectFace：从摄像头采集指定用户ID的人脸样本，保存到指定目录
 *          2. trainLBPHModel：遍历人脸样本目录，训练LBPH人脸识别模型并保存
 */
#include "face_tool.h"
#include <opencv2/face.hpp> // OpenCV人脸识别模块（LBPH算法）
#include <filesystem>       // C++17文件系统（遍历目录/创建文件夹）
#include <iostream>         // 标准输入输出（提示/错误信息）

namespace fs = std::filesystem;
using namespace cv;
using namespace cv::face;
using namespace std;

/**
 * @brief 人脸采集函数（核心实现）
 * @details 1. 根据传入的保存目录创建文件夹
 *          2. 打开摄像头，加载Haar人脸检测器
 *          3. 实时采帧、检测人脸并框选显示
 *          4. 按C键保存灰度人脸样本，按Q键退出
 * @param user_id 采集的用户ID（用于标识样本归属）
 * @param save_dir 人脸样本保存目录（如"face_data/1"）
 * @return bool 采集成功返回true，摄像头打开失败返回false
 */
bool collectFace(int user_id, const std::string& save_dir) {
    // 1. 递归创建样本保存目录
    fs::create_directories(save_dir);
    
    // 2. 打开默认摄像头（设备号0）
    VideoCapture cap(0, CAP_V4L2); // 强制V4L2后端，和门禁主程序保持一致
    if (!cap.isOpened()) return false;// 摄像头打开失败，直接返回false

    //设置采集分辨率（和门禁主程序匹配，避免训练/识别分辨率不一致）
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    
    // 3. 加载Haar人脸检测器
    CascadeClassifier face_cascade;// 声明Haar级联分类器对象
    std::string haar_path = "haarcascade_frontalface_alt.xml";
    if (!face_cascade.load(haar_path)) {
        haar_path = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";
        if (!face_cascade.load(haar_path)) {
            haar_path = "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";
            if (!face_cascade.load(haar_path)) {
                std::cerr << "Haar检测器加载失败!\n";
                return false;
            }
        }
    }
    
    // 4. 变量初始化
    Mat frame, gray;   // 摄像头彩色帧,灰度帧（人脸检测/训练需灰度图）
    vector<Rect> faces;// 检测到的人脸矩形区域
    int count = 0;     // 已保存的样本计数
    cout << "按 C 保存，按 Q 退出\n";
    
    // 5. 主循环：采帧→检测→显示→保存
    while (true) {
        // 读取摄像头帧
        cap >> frame;
        
        // 彩色帧转灰度帧（减少计算量，符合检测/训练要求）
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        
        // Haar人脸检测：参数（灰度图，人脸区域，缩放因子，邻域数，过滤规则，最小人脸尺寸）
        face_cascade.detectMultiScale(gray, faces, 1.1, 4, 0, Size(60, 60));
        
        // 绘制人脸框（绿色，线宽2，方便用户查看检测结果）
        for (auto& f : faces) rectangle(frame, f, Scalar(0,255,0), 2);
        imshow("人脸采集", frame);

        putText(frame, "已采集: " + to_string(count) + " 张", Point(10, 30), 
        FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255,0,0), 2);

        // 按键处理
        char key = waitKey(1);// 非阻塞等待按键（1ms）
        // 按C键且检测到人脸
        if (key == 'c' && !faces.empty()) {
            // 截取第一个人脸区域（灰度图）
            Mat face_img = gray(faces[0]);
            // 拼接保存路径：目录/face_计数.jpg
            string path = save_dir + "/face_" + to_string(count++) + ".jpg";
            imwrite(path, face_img);// 保存灰度人脸样本
            cout << "已保存: " << path << "\n";
        }
        if (key == 'q') break;// 按Q键退出采集
    }
    
    // 6. 资源释放
    cap.release();       // 释放摄像头
    destroyAllWindows(); // 关闭显示窗口
    return true;
}

/**
 * @brief LBPH人脸识别模型训练函数（核心实现）
 * @details 1. 遍历指定样本目录（按用户ID分文件夹）
 *          2. 读取所有用户的灰度人脸样本，关联对应的ID标签
 *          3. 训练LBPH模型，保存到指定路径
 * @param data_dir 人脸样本根目录（如"face_data"，下级为用户ID文件夹）
 * @param model_path 训练好的模型保存路径（如"lbph_model.yml"）
 * @return bool 训练成功返回true，无样本/训练失败返回false
 */
bool trainLBPHModel(const std::string& data_dir, const std::string& model_path) {
    // 1. 初始化样本容器：images存储人脸图像，labels存储对应用户ID
    vector<Mat> images;// 人脸样本图像集合
    vector<int> labels;// 样本对应的用户ID标签集合

    // 2. 遍历样本根目录（遍历每个用户ID文件夹）
    for (auto& user_dir : fs::directory_iterator(data_dir)) {
        // 跳过非目录文件（只处理用户ID文件夹）
        if (!user_dir.is_directory()) continue;
        
        // 获取用户ID（文件夹名转为数字）
        int id = stoi(user_dir.path().filename().string());
        
        // 遍历当前用户ID文件夹下的所有文件（即该用户的所有人脸样本）
        for (auto& img_file : fs::directory_iterator(user_dir.path())) {
            Mat img = imread(img_file.path().string(), 0);// 读取人脸样本图像
            if (img.empty()) continue;// 过滤空图像：跳过损坏/格式错误/无法读取的样本文件
            images.push_back(img);    // 将有效人脸样本添加到训练集
            labels.push_back(id);     // 将样本对应的用户ID标签添加到标签集
        }
    }

    // 检查训练集是否为空
    if (images.empty()) return false;

    // 创建LBPH人脸识别器实例
    Ptr<LBPHFaceRecognizer> model = LBPHFaceRecognizer::create();

    // 核心训练函数：输入样本集和标签集，训练LBPH模型
    // images：所有用户的灰度人脸样本集合（Mat类型数组）
    model->train(images, labels);

    // 将训练好的模型保存到指定路径
    // 保存路径示例："lbph_model.yml"，后续门禁系统通过g_lbph->read()加载ss
    model->save(model_path);
    return true;
}
#pragma once
#include <atomic>            // 原子变量，用于线程安全的运行状态控制
#include <thread>            // 多线程支持，创建各业务线程
#include <opencv2/opencv.hpp>// OpenCV核心库，处理图像/人脸检测/识别
#include "safe_queue.h"
#include "config.h"

/**
 * @class DoorCore
 * @brief 人脸识别门禁系统核心业务类
 * @details 采用多线程架构，将门禁系统拆分为4个独立线程：
 *          1. 采集线程：摄像头实时采帧，存入帧队列
 *          2. 检测线程：从帧队列取帧，检测人脸并存入人脸队列
 *          3. 识别线程：从人脸队列取人脸，进行身份识别并控制硬件
 *          4. 日志线程：处理系统日志，异步输出/保存
 * @note 所有线程通过线程安全队列通信，原子变量控制全局运行状态
 */
class DoorCore {
public:
    //构造函数,初始化成员变量，设置线程安全队列容量，原子变量初始化为false
    DoorCore();
    //析构函数,停止所有运行中的线程，释放资源，避免内存泄漏/线程残留
    ~DoorCore();
    //启动门禁系统（核心入口函数）
    void startSystem();

private:
    void captureThread();  //摄像头采集线程函数
    void detectThread();   //人脸检测线程函数
    void recognizeThread();//人脸识别线程函数
    void logThread();      //日志处理线程函数

    // ====================== 成员变量 ======================
    std::atomic<bool> is_running_{false};//系统运行状态标志（原子变量）

    std::thread cap_thread_;   //摄像头采集线程对象
    std::thread detect_thread_;//人脸检测线程对象
    std::thread rec_thread_;   //人脸识别线程对象
    std::thread log_thread_;   //日志处理线程对象
 
    SafeQueue<cv::Mat> frame_queue_{FRAME_QUEUE_SIZE};//帧队列（采集线程→检测线程）存储摄像头采集的原始帧，容量为FRAME_QUEUE_SIZE（3）避免帧堆积，满时采集线程丢弃新帧
    SafeQueue<cv::Mat> face_queue_{FACE_QUEUE_SIZE}; //人脸队列（检测线程→识别线程）存储检测到的人脸区域图像，容量为FACE_QUEUE_SIZE（3） 
};
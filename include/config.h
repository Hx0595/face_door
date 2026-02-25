#pragma once

//摄像头参数
constexpr int CAMERA_WIDTH = 640; //宽
constexpr int CAMERA_HEIGHT = 480;//长

//队列长度
constexpr int FRAME_QUEUE_SIZE = 3;//帧队列长度（用于缓存摄像头采集的图像帧）
constexpr int FACE_QUEUE_SIZE = 3; //人脸队列长度（用于缓存检测到的人脸数据）

//人脸识别阀值（小于该值表示识别成功）
constexpr double RECOGNIZE_THRESHOLD = 50.0;

// Haar 人脸检测器路径
constexpr const char* HAAR_PATH = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml";

// 模型路径
constexpr const char* MODEL_PATH = "lbph_model.yml";
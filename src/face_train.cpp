/**
 * @file face_train_main.cpp
 * @brief LBPH人脸识别模型训练工具主程序
 * @details 该程序作为模型训练功能的入口，指定人脸样本根目录和模型保存路径，
 *          调用trainLBPHModel函数自动遍历样本、训练模型并保存，适配门禁系统的模型更新流程
 */
#include "face_tool.h"
#include <iostream>

/**
 * @brief 主函数：LBPH模型训练工具入口
 * @return int 程序退出码（0表示正常退出，-1表示训练失败）
 */
int main() {
    // 1. 配置模型训练参数：指定人脸样本根目录,每个子文件夹存放对应用户的人脸灰度样本
    std::string data_dir = "/home/hexiang/face_door_system/face_data";
    
    // 2. 配置模型保存路径：训练完成后的LBPH模型将保存为该YAML文件
    std::string model_path = "lbph_model.yml";
    
    // 3. 调用模型训练核心函数，执行训练流程
    if (trainLBPHModel(data_dir, model_path)) {
        // 训练成功：打印提示信息，告知用户模型生成路径
        std::cout << "模型训练完成！已生成 " << model_path << "\n";
    } else {
        // 退出程序，返回-1表示训练流程异常
        std::cerr << "模型训练失败！\n";
        return -1;
    }

    // 4. 程序正常退出，返回0表示训练流程无异常
    return 0;
}
/**
 * @file face_collect_main.cpp
 * @brief 人脸样本采集工具主程序
 * @details 该程序作为人脸采集功能的入口，指定用户ID和样本保存目录，
 *          调用collectFace函数完成人脸样本采集，适配门禁系统的样本录入流程
 */
#include "face_tool.h"
#include <iostream>

/**
 * @brief 主函数：人脸采集工具入口
 * @return int 程序退出码（0表示正常退出，-1表示采集失败）
 */
int main() {
    // 1. 配置采集参数：指定待采集的用户ID
    int user_id = 1;

    // 2. 拼接样本保存目录路径,"face_data/用户ID"
    std::string save_dir = "face_data/" + std::to_string(user_id);
    
    // 3. 调用人脸采集核心函数，执行样本采集流程
    if (collectFace(user_id, save_dir)) {
        std::cout << "人脸采集完成！\n";// 采集成功：打印提示信息
    } else {
        std::cerr << "人脸采集失败！\n";// 采集失败：打印错误信息，并返回非0退出码（标识程序异常）
        return -1;// 退出程序，返回-1表示采集流程异常
    }
    
    // 4. 程序正常退出
    return 0;
}
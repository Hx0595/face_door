#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

/**
 * @brief 人脸采集核心函数
 * @param user_id 用户ID
 * @param save_dir 保存目录
 * @return 采集成功返回true
 */
bool collectFace(int user_id, const std::string& save_dir);

/**
 * @brief LBPH模型训练核心函数
 * @param data_dir 人脸数据目录
 * @param model_path 模型保存路径
 * @return 训练成功返回true
 */
bool trainLBPHModel(const std::string& data_dir, const std::string& model_path);
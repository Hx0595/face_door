#include "log_util.h"
#include <iostream>

SafeQueue<std::string> g_log_queue(50);// 定义全局日志队列，容量设为50

// 实现日志投递函数：将日志消息入队
void postLog(const std::string& msg) {
    g_log_queue.push(msg);
}
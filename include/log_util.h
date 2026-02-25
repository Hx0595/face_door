#pragma once
#include <string>
#include "safe_queue.h"

extern SafeQueue<std::string> g_log_queue;// 声明全局的日志队列

void postLog(const std::string& msg);// 声明日志投递函数：供外部调用，将日志消息放入队列
#include "door_core.h"

/**
 * @file main.cpp
 * @brief 人脸识别门禁系统主程序入口
 * @details 1. 创建DoorCore核心类实例（自动调用构造函数初始化资源）
 *          2. 调用startSystem()启动所有业务线程（采集/检测/识别/日志）
 *          3. 程序运行期间阻塞在startSystem()的循环中，直到手动终止
 */
int main() {
    //完成GPIO初始化、LBPH模型加载、日志初始化
    DoorCore door;
    // 启动门禁系统核心逻辑：
    // is_running_为true，启动4个业务线程（采集/检测/识别/日志）
    // 主线程进入阻塞循环（每秒检查is_running_状态），保持程序运行
    door.startSystem();
    // is_running_为false时，startSystem()退出循环，执行到此处
    // 停止所有队列、等待线程结束、释放资源
    return 0;
}
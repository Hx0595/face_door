#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

/*
*  通过队列解决多线程竞态条件问题
*  主线程：main() + startSystem()，只做 “启动 / 停止” 控制
*  摄像头采集线程：captureThread()，从摄像头读画面
*  人脸检测线程：detectThread()，从采集队列取画面 → 转灰度图 → 用 Haar 检测器找人脸 → 截取人脸区域
*  人脸识别线程：recognizeThread()，从检测队列取人脸区域 → 用 LBPH 模型识别 → 成功开门 / 失败报警
*  异步日志线程：logThread()，从日志队列取消息 → 输出到控制台 / 写入文件，不阻塞业务线程
*/

template <typename T>
class SafeQueue {
public:
    //指定队列最大容量（10），避免隐式类型转换
    explicit SafeQueue(size_t max_size = 10) : max_size_(max_size) {}
    //入队，非阻塞
    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(mtx_);// 互斥锁
        // 队列满则返回失败
        if (queue_.size() >= max_size_) return false;
        queue_.push(item);      // 入队数据
        cond_.notify_one();     // 唤醒一个等待的出队线程
        return true;            // 入队成功
    }
    //出队，阻塞
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);// 保证同一时间只有一个线程操作队列
        // 等待条件：队列非空 OR 收到停止信号
        cond_.wait(lock, [this]() {
            return !queue_.empty() || stop_flag_;// 条件变量：队列空时休眠，有数据再唤醒
        });
        // 停止且队列为空：返回失败（线程可退出）
        if (stop_flag_ && queue_.empty()) return false;
        // 取出队首元素并弹出
        item = queue_.front();
        queue_.pop();
        return true;
    }
    //主动停止
    /*主动触发所有阻塞在pop方法的线程退出等待*/
    void stop() {
        std::lock_guard<std::mutex> lock(mtx_);// 轻量级加锁（lock_guard自动释放）
        stop_flag_ = true;                     // 设置停止标志
        cond_.notify_all();                    // 唤醒所有等待的出队线程
    }

private:
    std::queue<T> queue_;          // 底层存储队列，存放实际数据
    mutable std::mutex mtx_;       // 互斥锁，保护所有队列操作
    std::condition_variable cond_; // 条件变量，用于阻塞/唤醒出队线程
    size_t max_size_;              // 队列最大容量，限制队列不会无限膨胀
    bool stop_flag_ = false;       // 停止标志，用于终止出队线程的等待
};
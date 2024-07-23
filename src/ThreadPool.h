#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <sys/syscall.h>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool
{
private:
    std::vector<std::thread> m_threads;            // 线程池中的线程。
    std::queue<std::function<void()>> m_taskQueue; // 任务队列。
    std::mutex m_mutex;                            // 任务队列同步的互斥锁。
    std::condition_variable m_condition;           // 任务队列同步的条件变量。
    std::atomic_bool m_stop;                       // 在析构函数中，把stop_的值设置为true，全部的线程将退出。
    std::string m_threadType;                      // 线程种类"IO"、"WORKS"
public:
    // 在构造函数中将启动threadnum个线程，
    ThreadPool(size_t threadNum,const std::string & threadType);

    // 把任务添加到队列中。
    void addTask(std::function<void()> task);
    // 获取线程池大小
    size_t size();
    //停止线程
    void stopThread();
    // 在析构函数中将停止线程。
    ~ThreadPool();
};
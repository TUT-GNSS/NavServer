#pragma once
#include "Epoll.h"
#include <functional>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h>
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <map>
#include  "Connection.h"
#include <queue>
#include <atomic>
#include <condition_variable>

class Connection;
class Channel;
class Epoll;

using s_ptrConnection = std::shared_ptr<Connection>; // 给Connection类型的智能指针其别名为s_ptrConnection

// 事件循环类
class EventLoop
{
private:
    std::unique_ptr<Epoll> m_ep;                             // 每个事件循环只有一个Epoll
    int m_timeInterval;                                      // 闹钟时间间隔，单位 秒
    int m_timeout;                                           // Connection对象超时时间，单位 秒
    std::function<void(EventLoop *)> m_epollTimeoutCallback; // epoll_wait()超时的回调函数
    pid_t m_threadID;                                        // 事件循环所在线程ID
    std::queue<std::function<void()>> m_taskQueue;           // 事件循环线程被eventfd唤醒后执行的任务队列
    std::mutex m_taskQueueMutex;                             // 任务对列同步的互斥锁
    int m_wakeupfd;                                          // 用于唤醒事件循环线程的eventfd
    std::unique_ptr<Channel> m_wakeupChannel;           //eventfd的Channel
    int m_timerfd;                                                  //定时器的fd
    std::unique_ptr<Channel> m_timerChannel;//定时器的Channel
    bool m_isMainLoop;                                           //true是主事件循环，false从事件循环
    std::mutex m_elConnectionMapMutex;                                // 保护m_elConnectionMap的互斥锁
    std::map<int, s_ptrConnection> m_elConnectionMap; //存放运行在该事件上全部Connection对象
    std::queue<int> m_timeoutFdQueue;                   // 存放m_elConnectionMap中超时的Connection的fd
    std::function<void(int)> m_timeoutRemoveConnCallback;        // 删除TcpServer中超时的Connection对象，将被设置为TcpServer::removeConnection();
    std::atomic_bool m_stop;

public:
    EventLoop(bool isMainLoop,int timeInterval=30,int timeout=80); // 在构造 函数中创建Epoll对象m_ep
    ~EventLoop(); // 在析构函数中销毁m_ep

    void run();                                                        // 运行事件循环
    void stop();                                                      // 停止事件循环
    Epoll *getEpoll();                                                 // 得到m_ep成员
    void updateChannel(Channel *ch);                                   // 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
    void removeChannel(Channel *ch);                                   // 把channel从红黑树删除
    void setEpollTimeoutCallback(std::function<void(EventLoop *)> fn); // epoll_wait()的回调函数

    bool isInLoopThread();                               // 判断当前线程是否为事件循环线程
    void addTaskToQueueInLoop(std::function<void()> fn); // 把任务添加到队列中
    void wakeupEventLoop();                              // 用eventfd唤醒事件循环线程
    void handleWakeup();                                 // 事件循环被eventfd唤醒后执行的函数

    void handleTimer();                                       //闹钟响时执行的函数

    // 把Connection对象保存在m_ConnectionMap中
    void addNewConnection(s_ptrConnection conn);

    void setTimeoutRemoveConnCallback(std::function<void(int)> fn);
};
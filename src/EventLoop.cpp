#include "EventLoop.h"

int createTimerfd(int sec = 30)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK); // 创建timerfd。
    struct itimerspec timeout;                                             // 定时时间的数据结构。
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = sec; 
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0);
    return tfd;
}


// 在构造 函数中创建Epoll对象m_ep
EventLoop::EventLoop(bool isMainLoop,int timeInterval , int timeout ) : m_ep(new Epoll), m_isMainLoop(isMainLoop),
                                        m_timeInterval(timeInterval),m_timeout(timeout),m_stop(false),
                                        m_wakeupfd(eventfd(0, EFD_NONBLOCK)), 
                                        m_wakeupChannel(new Channel(this, m_wakeupfd)), m_timerfd(createTimerfd(m_timeout)), 
                                        m_timerChannel(new Channel(this, m_timerfd))
{
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleWakeup, this));
    m_wakeupChannel->enableReading();

    m_timerChannel->setReadCallback(std::bind(&EventLoop::handleTimer, this));
    m_timerChannel->enableReading();
}

EventLoop::~EventLoop() // 在析构函数中销毁m_ep
{
    // delete m_ep;
}



void EventLoop::run() // 运行事件循环
{
    // printf("EventLoop::run() thread is %ld\n", syscall(SYS_gettid));
    m_threadID = syscall(SYS_gettid);       //获取事件循环所在线程得ID

    while (m_stop==false) // 事件循环。
    {
        std::vector<Channel *> channels = m_ep->loop(10 * 1000); // 等待监视的fd有事件发生。
        // 如果channels为空，说明超时，回调TcpServer::epollTimeout
        if (channels.size() == 0)
        {
            m_epollTimeoutCallback(this);
        }
        else
        {
            for (auto &ch : channels)
            {
                ch->handleEvent(); // 处理epoll_wait()返回的事件。
            }
        }
    }
}
// 停止事件循环
void EventLoop::stop()
{
    m_stop = true;
    wakeupEventLoop();//立即唤醒事件循环，不然epoll_wait()还阻塞

}

// 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
void EventLoop::updateChannel(Channel *ch)
{
    m_ep->updateChannel(ch);
}
// 把channel从红黑树删除
void EventLoop::removeChannel(Channel *ch)
{
    m_ep->removeChannel(ch);
}

void EventLoop::setEpollTimeoutCallback(std::function<void(EventLoop *)> fn) // epoll_wait()的回调函数
{
    m_epollTimeoutCallback = fn;
}

// 判断当前线程是否为事件循环线程
bool EventLoop::isInLoopThread()
{
    return m_threadID == syscall(SYS_gettid);
}

// 把任务添加到队列中
void EventLoop::addTaskToQueueInLoop(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> gd(m_taskQueueMutex); // 给任务对列加锁
        m_taskQueue.push(fn);
    }
    //唤醒事件循环
    wakeupEventLoop();

}

// 用eventfd唤醒事件循环线程
void EventLoop::wakeupEventLoop()
{
    uint64_t val = 1;
    write(m_wakeupfd, &val, sizeof(val));
}

// 事件循环被eventfd唤醒后执行的函数
void EventLoop::handleWakeup() 
{

    // printf("handleWakeup() thread id is %ld.\n", syscall(SYS_gettid));
    uint64_t val;
    read(m_wakeupfd, &val, sizeof(val));    //从eventfd中读取出数据，如果不读取，eventfd的读事件会一直触发

    std::function<void()> fn;

    std::lock_guard<std::mutex> gd(m_taskQueueMutex); // 给任务对列加锁
    // 执行队列中全部发送的任务
    while (!m_taskQueue.empty())
    {
            fn = std::move(m_taskQueue.front());              // 出队一个元素
            fn(); // 执行任务
            m_taskQueue.pop();
    }

}

//闹钟响时执行的函数
void EventLoop::handleTimer()
{
    //重新计时
    struct itimerspec timeout; // 定时时间的数据结构。
    memset(&timeout, 0, sizeof(struct itimerspec));
    timeout.it_value.tv_sec = m_timeInterval; 
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(m_timerfd, 0, &timeout, 0);
    if(m_isMainLoop){
        //printf("主事件循环闹钟时间到了\n");

    }
    else{
        //printf("从事件循环闹钟时间到了\n");
        // printf("EventLoop::handletimer()thread is(%ld).", syscall(SYS_gettid));
        time_t now = time(0);

        for (auto conn : m_elConnectionMap)
        {
            // printf("%d", conn.first);
            if(conn.second->hasTimeout(now,m_timeout)){
                // m_elConnectionMap.erase(conn.first);      //从map中删除超时的conn
                {
                    std::lock_guard<std::mutex> gd(m_elConnectionMapMutex);
                    m_timeoutFdQueue.push(conn.first); // 将超时的fd存入m_timeoutFdQueue
                }
                
            }
        }
        // 将m_timeoutFdQueue中超时的fd对应的conn从m_elConnectionMap和TcpServer中m_ConnectionMap中删除
        
            while (!m_timeoutFdQueue.empty())
            {
                {
                    std::lock_guard<std::mutex> gd(m_elConnectionMapMutex);
                    m_elConnectionMap.erase(m_timeoutFdQueue.front()); // 从m_elConnectionMap中删除
                }
                m_timeoutRemoveConnCallback(m_timeoutFdQueue.front()); // 从TcpServer中m_ConnectionMap中删除
                m_timeoutFdQueue.pop();
            }

    }
}

void EventLoop::addNewConnection(s_ptrConnection conn) // 把Connection对象保存在m_ConnectionMap中
{
        std::lock_guard<std::mutex> gd(m_elConnectionMapMutex);
        m_elConnectionMap[conn->fd()] = conn;
}

void EventLoop::setTimeoutRemoveConnCallback(std::function<void(int)> fn)
{
    m_timeoutRemoveConnCallback = fn;
}
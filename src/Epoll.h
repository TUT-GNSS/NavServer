#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include "Channel.h"

class Channel;

// Epoll类。
class Epoll
{
private:
    static const int MaxEvents=100;                   // epoll_wait()返回事件数组的大小。
    int m_epollfd=-1;                                             // epoll句柄，在构造函数中创建。
    epoll_event m_events[MaxEvents];                  // 存放poll_wait()返回事件的数组，在构造函数中分配内存。
public:
    Epoll();                                             // 在构造函数中创建了m_epollfd。
    ~Epoll();                                          // 在析构函数中关闭m_epollfd。

    void updateChannel(Channel *ch);           // 把Channel添加/更新到红黑树上，Channel中有fd，也有需要监视的事件。
    // std::vector<epoll_event> loop(int timeout=-1);   // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
    void removeChannel(Channel *ch);        //从红黑树删除Channel
    std::vector<Channel *> loop(int timeout = -1); // 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
};
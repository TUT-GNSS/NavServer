#include "Epoll.h"

Epoll::Epoll() // 在构造函数中创建了epollfd_。
{
    if ((m_epollfd = epoll_create(1)) == -1)
    { // 创建epoll句柄（红黑树）。
        printf("epoll_create() faile(%d).\n", errno);
        exit(-1);
    }
}

Epoll::~Epoll()
{ // 在析构函数中关闭epollfd_。
    ::close(m_epollfd);
}

void Epoll::updateChannel(Channel *ch) // 把Channel添加/更新到红黑树上，Channel中有fd，也有需要监视的事件。
{
    epoll_event ev;           // 声明事件的数据结构
    ev.data.ptr = ch;         // 指定Channel
    ev.events = ch->events(); // 指定事件

    if (ch->inpoll())
    { // 如果Channel已经在树上
        if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() faile\n.");
            exit(-1);
        }
    }
    else
    { // 如果Channel不在树上
        if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() faile\n.");
            exit(-1);
        }
        ch->setInepoll(true); // 把Channel的m_inepoll成员设置为true
    }
}
 // 把channel从红黑树删除
void Epoll::removeChannel(Channel *ch)
{
    if(ch->inpoll()){
        if(epoll_ctl(m_epollfd,EPOLL_CTL_DEL,ch->fd(),NULL)==-1){
            perror("epoll_ctl_DEL() faile\n.");
            exit(-1);
        }
    }
    ch->setInepoll(false);
}

std::vector<Channel *> Epoll::loop(int timeout)
{
    std::vector<Channel *> channels;

    // for (auto &ev : m_events)
    // {
    //     if (ev.events & EPOLLIN)
    //     {
    //         printf("IN!!!!!\n");
    //     }
    //     else if (ev.events & EPOLLOUT)
    //     {
    //         printf("OUT!!!!!\n");
    //     }
    // }
    bzero(m_events, sizeof(m_events));

    int infds = epoll_wait(m_epollfd, m_events, MaxEvents, timeout); // 等待监视的fd有事件发生。
    // printf("infds:(%d)\n", infds);


    // 返回失败。
    if (infds < 0)
    {
        // EBADF ：epfd不是一个有效的描述符。
        // EFAULT ：参数events指向的内存区域不可写。
        // EINVAL ：epfd不是一个epoll文件描述符，或者参数maxevents小于等于0。
        // EINTR ：阻塞过程中被信号中断，epoll_pwait()可以避免，或者错误处理中，解析error后重新调用epoll_wait()。
        // 在Reactor模型中，不建议使用信号，因为信号处理起来很麻烦，没有必要。------ 陈硕
        perror("epoll_wait() failed");
        exit(-1);
    }

    // 超时。
    if (infds == 0)
    {
        return channels;
    }

    // 如果infds>0，表示有事件发生的fd的数量。
    for (int i = 0; i < infds; i++) // 遍历epoll返回的数组m_events。
    {
        Channel *ch = (Channel *)m_events[i].data.ptr; // 取出已经发生事件的Channel
        ch->setRevents(m_events[i].events);            // 设置Channel的m_revents成员

        channels.push_back(ch);
    }
    return channels;
}

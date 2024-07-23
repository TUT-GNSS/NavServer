#include "Channel.h"

Channel::Channel(EventLoop *loop, int fd) : m_loop(loop), m_fd(fd)
{
}

Channel::~Channel()
{
    // 在析构函数中，不要销毁m_ep，也不能关闭m_fd，因为这两个东西不属于Channel类，Channel类只是使用它们而已
}

int Channel::fd() // 返回m_fd成员
{
    return m_fd;
}
void Channel::useEt() // 采用边缘触发模式
{
    m_events = m_events | EPOLLET;
}
void Channel::enableReading() // 让epoll_wait()监视m_fd的读事件
{
     m_events |= EPOLLIN;
    m_loop->updateChannel(this);
}
void Channel::disableReading()   //取消读事件
{
    m_events &=~EPOLLIN;
    m_loop->updateChannel(this);
}
void Channel::enableWriting()   //注册写事件
{
    m_events |= EPOLLOUT;
    m_loop->updateChannel(this);
}
void Channel::disableWriting()   //取消写事件
{
    m_events &= ~EPOLLOUT;
    m_loop->updateChannel(this);

}
// 取消所有事件
void Channel::disableAllEvents()
{
    m_events = 0;
    m_loop->updateChannel(this);
}
// 从事件循环中删除Channel
void Channel::removeChannel()
{
    disableAllEvents();                             //先取消全部事件
    m_loop->removeChannel(this);        //从红黑树中删除fd
}
void Channel::setInepoll(bool in) // 把m_inepoll成员的值设置为true
{
    m_inepoll = in;
}
void Channel::setRevents(uint32_t ev) // 设置m_revents成员的值为参数ev
{
    m_revents = ev;
}
bool Channel::inpoll() // 返回m_inepoll成员
{
    return m_inepoll;
}
uint32_t Channel::events() // 返回m_events成员
{
    return m_events;
}
uint32_t Channel::revents() // 返回m_revents成员
{
    return m_revents;
}

void Channel::handleEvent() // 事件处理函数，epoll_wait()返回时候，执行它
{
    if (m_revents & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    {
        m_closeCallback(); // 调用m_closeCallback回调函数
    } //  普通数据  带外数据
    else if (m_revents & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
    {
        // printf("(EPOLLIN | EPOLLPRI)\n");
        m_readCallback();
    }
    else if (m_revents & EPOLLOUT) // 有数据需要写。
    {
        // printf("(EPOLLOUT\n");
        m_writeCallback(); // 回调Connection::writeCallback()
    }
    else // 其它事件，都视为错误。
    {
        m_errorCallback(); // 调用m_errorCallback回调函数
    }
} 

void Channel::setReadCallback(std::function<void()> fn)     //设置fd读事件的回调函数
{
    m_readCallback = fn;
}

void Channel::setCloseCallback(std::function<void()> fn) // 设置关闭fd_的回调函数。
{
    m_closeCallback = fn;
}

void Channel::setErrorCallback(std::function<void()> fn) // 设置fd_发生了错误的回调函数。
{
    m_errorCallback = fn; 
}

void Channel::setWriteCallback(std::function<void()> fn)    // 设置m_fd写事件的回调函数。
{
    m_writeCallback = fn;
}


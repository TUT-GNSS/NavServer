#pragma once
#include <sys/epoll.h>
#include <functional>
#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include <memory>


class EventLoop;

class Channel
{
private:
    int m_fd=-1;        //Channel拥有的fd，Channel和fd是一对一的关系
    EventLoop *m_loop = nullptr; // Channel对应的红黑树，Channel与EventLoop是多对一的关系，一个Channel只对应一个EventLoop
    bool m_inepoll = false;     // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD。
    uint32_t m_events = 0;      // m_fd需要监视的事件。listenfd和clientfd需要监视EPOLLIN，clientfd还可能需要监视EPOLLOUT。
    uint32_t m_revents = 0;     // m_fd已发生的事件。

    std::function<void()> m_readCallback; // m_fd读事件的回调函数,如果是acceptChannel，回调Acceptor::newConnection(),如果是clientChannel,回调Channel::onMessage()
    std::function<void()> m_closeCallback; // 关闭m_fd的回调函数，将回调Connection::closecallback()。
    std::function<void()> m_errorCallback; // m_fd发生了错误的回调函数，将回调Connection::errorcallback()。
    std::function<void()> m_writeCallback; // m_fd写事件的回调函数，将回调Connection::writeCallback()。
public:
    Channel(EventLoop *loop, int fd); // Channel是Acceptor和Connection的下层类
    ~Channel();

    int fd();       //返回m_fd成员
    void useEt();       //采用边缘触发模式
    void enableReading();   //让epoll_wait()监视m_fd的读事件
    void disableReading();   //取消读事件
    void enableWriting();   //注册写事件
    void disableWriting();   //取消读事件
    void disableAllEvents();    //取消所有事件
    void removeChannel();    //从事件循环中删除Channel
    void setInepoll(bool in);              // 把m_inepoll成员的值设置为true
    void setRevents(uint32_t ev);   //设置m_revents成员的值为参数ev
    bool inpoll();      //返回m_inepoll成员
    uint32_t events();      //返回m_events成员
    uint32_t revents();      //返回m_revents成员

    void handleEvent();     //事件处理函数，epoll_wait()返回时候，执行它

    void setReadCallback(std::function<void()> fn);     //设置m_fd读事件的回调函数
    void setCloseCallback(std::function<void()> fn);    // 设置关闭m_fd_的回调函数。
    void setErrorCallback(std::function<void()> fn);    // 设置m_fd发生了错误的回调函数。
    void setWriteCallback(std::function<void()> fn);    // 设置m_fd写事件的回调函数。
    
};
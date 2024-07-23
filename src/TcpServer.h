#pragma once
#include "EventLoop.h"
#include "Socket.h"
#include "Channel.h"
#include "Acceptor.h"
#include "Connection.h"
#include <map> 
#include <vector>
#include "ThreadPool.h"
#include <mutex>
#include <memory>

// TCP网络服务类
class TcpServer
{
private:
    std::unique_ptr<EventLoop> m_mainLoop;       //主事件循环,一个TcpServer可以有多个事件循环
    std::vector<std::unique_ptr<EventLoop>> m_subLoops; // 存放从事件循环的容器
    Acceptor m_acceptor;                               // 一个TcpServer只有一个Acceptor对象
    int m_threadNum;                                   // 线程池的大小，事件循环的个数 
    ThreadPool m_threadPool;       //线程池
    std::mutex m_connMapMutex;     // 保护m_ConnectionMap的互斥锁
    std::map<int, s_ptrConnection> m_ConnectionMap; // 存放创建的Connection对象
    std::function<void(s_ptrConnection)> m_newConnectionCallback;
    std::function<void(s_ptrConnection)> m_closeConnectionCallback;
    std::function<void(s_ptrConnection)> m_errorConnectionCallback;
    std::function<void(s_ptrConnection, std::string &)> m_handleMessageCallback;
    std::function<void(s_ptrConnection)> m_sendCompleteCallback;
    std::function<void(EventLoop*)> m_epollTimeout;
    std::function<void(int)> m_handleConnectionTimeoutCallback;

public:
    TcpServer(const std::string &ip,const uint16_t &port,int threadNum=3);
    ~TcpServer();

    void startLoopRun();     //运行事件循环
    void stopLoopRun();     //停止IO线程和事件循环

    void newConnection(std::unique_ptr<Socket> clientSock); // 处理新客户端连接请求
    void closeConnection(s_ptrConnection conn); // 关闭客户端的连接，在Connection类中回调此函数
    void errorConnection(s_ptrConnection conn); // 客户端连接错误，在Connection类中回调此函数
    void handleMessage(s_ptrConnection conn, std::string &message); // 处理客户端的请求报文，在Connection类中回调此函数
    void sendComplete(s_ptrConnection conn);  // send发送成功后的回调函数，在Connection类中回调此函数
    void epollTimeout(EventLoop *loop);       // epoll_wait超时，在EventLoop类中回调此函数

    void setNewConnection(std::function<void(s_ptrConnection)> fn);
    void setCloseConnection(std::function<void(s_ptrConnection)> fn);
    void setErrorConnection(std::function<void(s_ptrConnection)> fn);
    void setHandleMessage(std::function<void(s_ptrConnection, std::string &)> fn);
    void setSendComplete(std::function<void(s_ptrConnection)> fn);
    void setEpollTimeout(std::function<void(EventLoop *)> fn);
    void setHandleConnectionTimeout(std::function<void(int)> fn);

    void timeoutRemoveConnection(int fd); // 删除m_ConnectionMap中的Connection对象，在EventLoop::handleTimer()中将回调此函数
};
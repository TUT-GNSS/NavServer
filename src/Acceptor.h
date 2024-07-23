#pragma once
#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include <memory>

class Acceptor
{
private:
    EventLoop *m_loop;         // Acceptor对应的事件循环，在构造函数中传入,由于Acceptor只是使用不是创建，因此采用重引用
    Socket m_serverSock;       //服务端用于监听socket，在构造函数中创建,占用内存不大，可以直接放在栈中
    Channel m_acceptorChannel;                             // Acceptor对应的Channel，在构造函数中创建,占用内存不大，可以直接放在栈中
    std::function<void(std::unique_ptr<Socket>)> m_newConnectionCallback; // 处理新客户端连接请求的回调函数，将指向TcpServer::newConnection()

public:
    Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port);
    ~Acceptor();

    void newConnection();     //处理新客户端连接请求。

    void setNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> fn); // 处理新客户端连接请求的回调函数
};
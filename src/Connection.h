#pragma once
#include <functional>
#include "Socket.h"
#include "InetAddress.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include <memory>
#include <atomic>
#include <sys/syscall.h>
#include "TimeStamp.h"
#include <iostream>



class Channel;
class EventLoop;
class Connection;
using s_ptrConnection = std::shared_ptr<Connection>; // 给Connection类型的智能指针其别名为s_ptrConnection

// Connetion继承模板类std::__enable_shared_from_this,可以使用shared_from_this()返回自身对象的智能指针
class Connection:public std::enable_shared_from_this<Connection>
{
private:

    EventLoop *m_loop;                          // Connetion对应的事件循环，在构造函数中传入
    std::unique_ptr<Socket> m_clientSock;       //服务端用于监听socket，在构造函数中创建
    std::unique_ptr<Channel> m_clientChannel;     //Connection对应的Channel，在构造函数中创建

    Buffer m_inputBuffer;       //接收缓冲区
    Buffer m_outputBuffer;    //发送缓冲区

    std::atomic_bool m_disconnect;      //客户端连接是否断开，如果断开设置为true

    std::function<void(s_ptrConnection)> m_closeCallback; // 关闭fd_的回调函数，将回调Connection::closecallback()。
    std::function<void(s_ptrConnection)> m_errorCallback; // fd_发生了错误的回调函数，将回调Connection::errorcallback()。
    std::function<void(s_ptrConnection, std::string &)> m_handleMessageCallback; // 处理报文的回调函数，将回调TcpServer::handleMessage
    std::function<void(s_ptrConnection)> m_sendCompleteCallback; // 发送完成的回调函数，将回调TcpServer::sendComplete
    TimeStamp m_lastTime;       //时间戳，创建Connection对象时为当前时间，每接收到一个报文，时间戳更新为当前时间

public:
    Connection(EventLoop *loop, std::unique_ptr<Socket> clientSock);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void closeCallback(); //Tcp连接关闭（断开）的回调函数，供Channel回调
    void errorCallback();//Tcp连接错误的回调函数，供Channel回调
    void writeCallback();//Tcp触发写事件的回调函数，供Channel回调

    void handleMessage();    // 处理对端发送过来的消息

    void setCloseCallback(std::function<void(s_ptrConnection)> fn);
    void setErrorCallback(std::function<void(s_ptrConnection)> fn);
    void setHandleMessageCallback(std::function<void(s_ptrConnection, std::string &)> fn);
    void setSendCompleteCallback(std::function<void(s_ptrConnection)> fn);

    void sendMessage(const char *data, size_t sz);   //发送数据,不管在任何线程中，都调用此函数发送数据
    void sendInLoop(const char*data,size_t sz); //发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将此函数传给IO线程

    bool hasTimeout(time_t now,int interval);   // 判断当前时间是否超时(空闲太长时间)
};
#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "InetAddress.h"

//创建非阻塞的socket
int createNonBlocking();

// socket 类
class Socket
{
private:
    const int m_fd;      //Socket持有的fd,在构造函数中传进来
    std::string m_ip;    //listenfd：存放服务端监听的ip。客户端连接的fd：存放对端ip
    uint16_t m_port;  //listenfd：存放服务端监听的端口。客户端连接的fd：存放对端端口

public:
    Socket(int fd);     
    ~Socket();          

    int fd() const;     //返回m_fd成员
    std::string ip() const;   //返回ip地址
    uint16_t port() const; //返回端口号

    void setIPandPort(const std::string &ip, uint16_t port);   //设置m_ip和m_port
    void setreuseaddr(bool on);       // 设置SO_REUSEADDR选项，true-打开，false-关闭。
    void setreuseport(bool on);       // 设置SO_REUSEPORT选项。
    void settcpnodelay(bool on);     // 设置TCP_NODELAY选项。
    void setkeepalive(bool on);       // 设置SO_KEEPALIVE选项。
    void bind(const InetAddress& servaddr);        // 服务端的socket将调用此函数。
    void listen(int nn=128);                                    // 服务端的socket将调用此函数。
    int   accept(InetAddress& clientaddr);            // 服务端的socket将调用此函数。void setresuseaddr()
};
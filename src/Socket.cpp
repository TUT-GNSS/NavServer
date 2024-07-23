#include "Socket.h"

int createNonBlocking(){
    //创建服务端用于监听的listenfd
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(listenfd<0){
        // perror("socket() falied");
        printf("%s:%s:%d listen socket create error:%d\n", __FILE__, __FUNCTION__, __LINE__, errno); 
        exit(-1);
        }
    return listenfd;
}
Socket::Socket(int fd):m_fd(fd){};

Socket::~Socket(){
    ::close(m_fd);
}

int Socket::fd() const                              // 返回fd_成员。
{
    return m_fd;
}

void Socket::settcpnodelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setreuseaddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); 
}

void Socket::setreuseport(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); 
}

void Socket::setkeepalive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); 
}
void Socket::bind(const InetAddress& servaddr){
    if (::bind(m_fd, servaddr.addr(), sizeof(servaddr)) < 0)
    {
        perror("bind() failed");
        close(m_fd);
        exit(-1);
    }
    //监听的listenfd赋值ip和端口
    setIPandPort(servaddr.ip(), servaddr.port());
}

void Socket::listen(int nn){
    if (::listen(m_fd,nn) != 0 )        // 在高并发的网络服务器中，第二个参数要大一些。
    {
        perror("listen() failed");
        close(m_fd);
        exit(-1);
    }
}

int Socket::accept(InetAddress &clientAddr){
    sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);
    int clientfd = accept4(m_fd, (struct sockaddr *)&peerAddr, &len, SOCK_NONBLOCK);

    clientAddr.setAddr(peerAddr); // 客户端的地址和协议
     
    return clientfd;
}

std::string Socket::ip() const   //返回ip地址
{
    return m_ip;
}
uint16_t Socket::port() const //返回端口号
{
    return m_port;
}

void Socket::setIPandPort(const std::string &ip, uint16_t port) // 设置m_ip和m_port
{
    m_ip = ip;
    m_port = port;
}
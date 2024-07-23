#include "Acceptor.h"

Acceptor::Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port) : m_loop(loop), m_serverSock(createNonBlocking()), m_acceptorChannel(loop, m_serverSock.fd())
{
     // 创建服务端用于监听的listenfd。
    // m_serverSock=new Socket(createNonBlocking());

    InetAddress servaddr(ip, port); // 服务端的地址和协议

    // 设置listenfd的属性。
    m_serverSock.settcpnodelay(true); // 必须的。
    m_serverSock.setreuseaddr(true);  // 必须的。
    m_serverSock.setreuseport(true);  // 有用，但是，在Reactor中意义不大。
    m_serverSock.setkeepalive(true);  // 可能有用，但是，建议自己做心跳。

    m_serverSock.bind(servaddr); // 绑定服务端的地址和协议给服务端监听的socket

    m_serverSock.listen(); // 设置用于监听的socket，在高并发的网络服务器中，第二个参数要大一些。


    // m_acceptorChannel = new Channel(loop, m_serverSock.fd()); 
    m_acceptorChannel.setReadCallback(std::bind(&Acceptor::newConnection, this));
    m_acceptorChannel.enableReading(); // 让epoll_wait()监视serverChannel的读事件

}

void Acceptor::newConnection() // 处理新客户端连接请求
{
    InetAddress clientaddr; // 客户端的地址和协议
    
    std::unique_ptr<Socket> clientSock (new Socket(m_serverSock.accept(clientaddr)));
    clientSock->setIPandPort(clientaddr.ip(), clientaddr.port());
    m_newConnectionCallback(std::move(clientSock));
}

Acceptor::~Acceptor(){
    // delete m_serverSock;
    // delete m_acceptorChannel;
}

void Acceptor::setNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> fn) // 处理新客户端连接请求的回调函数
{
    m_newConnectionCallback = fn;
}
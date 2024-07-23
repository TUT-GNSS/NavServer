#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, const uint16_t &port, int threadNum) : m_threadNum(threadNum),
                         m_mainLoop(new EventLoop(true)), m_acceptor(m_mainLoop.get(), ip, port), 
                         m_threadPool(m_threadNum, "IO")
{
    // m_mainLoop = new EventLoop;//创建主事件循环
    m_mainLoop->setEpollTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));

    // m_acceptor =new Acceptor(m_mainLoop, ip, port);
    m_acceptor.setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,std::placeholders::_1));

    // m_threadPool = new ThreadPool(m_threadNum,"IO");     //创建线程池

    //创建从事件循环
    for (int i = 0; i < threadNum;++i){
        m_subLoops.emplace_back(new EventLoop(false,5,10));            //创建从事件循环，放入m_subLoops中
        m_subLoops[i]->setEpollTimeoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));   //设置从事件循环的超时回调函数
        m_subLoops[i]->setTimeoutRemoveConnCallback(std::bind(&TcpServer::timeoutRemoveConnection, this, std::placeholders::_1));//设置从事件循环超时后从m_ConnectionMap中删除Connection对象的回调函数
        m_threadPool.addTask(std::bind(&EventLoop::run, m_subLoops[i].get()));
    }
}

void TcpServer::startLoopRun(){
    m_mainLoop->run();
}

void TcpServer::stopLoopRun(){
    //停止主事件循环
    m_mainLoop->stop();
    printf("主事件循环停止\n");
    // 停止从事件循环
    for (int i = 0; i < m_threadNum;++i)
    {
        m_subLoops[i]->stop();
    }
    printf("从事件循环停止\n");
    //停止IO线程
    m_threadPool.stopThread();
    printf("IO线程池已停止\n");
}

TcpServer::~TcpServer()
{
    // delete m_acceptor;
    // delete m_mainLoop;
    // 释放m_ConnectionMap中全部的Connection对象
    // for(auto &con:m_ConnectionMap){
    //     delete con.second;
    // }

//    //释放从事件循环
//    for(auto &subloop:m_subLoops){
//        delete subloop;
//    }
   //释放线程池
//    delete m_threadPool;
}

void TcpServer::newConnection(std::unique_ptr<Socket> clientSock) // 处理新客户端连接请求。
{
    // Connection *conn = new Connection(m_mainLoop, clientSock);     //new出没释放
    s_ptrConnection conn(new Connection(m_subLoops[clientSock->fd() % m_threadNum].get(),
                                         std::move(clientSock))); 
    conn->setCloseCallback(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    conn->setErrorCallback(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));
    conn->setHandleMessageCallback(std::bind(&TcpServer::handleMessage, this, std::placeholders::_1, std::placeholders::_2));
    conn->setSendCompleteCallback(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));

    // printf("new connection(fd=%d,ip=%s,port=%d) ok.\n", conn->fd(), conn->ip().data(), conn->port());
    {
        std::lock_guard<std::mutex> gd(m_connMapMutex);
        m_ConnectionMap[conn->fd()] = conn; // 把conn存放到TcpServer的map容器中
    }
    m_subLoops[conn->fd() % m_threadNum]->addNewConnection(conn);//把conn存放到EventLoop的map容器中

    if (m_newConnectionCallback)
    {
        m_newConnectionCallback(conn);
    }
}

void TcpServer::closeConnection(s_ptrConnection conn) // 关闭客户端的连接，在Connection类中回调此函数
{
    if (m_closeConnectionCallback)
    {
        m_closeConnectionCallback(conn);
    } 
    // printf("client(eventfd=%d) disconnected.\n", conn->fd());
    {
        std::lock_guard<std::mutex> gd(m_connMapMutex);
        m_ConnectionMap.erase(conn->fd()); // 从map中删除conn
    }
        // delete conn;//释放Connection对象
}
void TcpServer::errorConnection(s_ptrConnection conn) // 客户端连接错误，在Connection类中回调此函数
{
    if (m_errorConnectionCallback){
        m_errorConnectionCallback(conn);
    }
    printf("client(eventfd=%d) error.\n", conn->fd());
    {
        std::lock_guard<std::mutex> gd(m_connMapMutex);
        m_ConnectionMap.erase(conn->fd()); // 从map中删除conn
    }
    // delete conn;                       // 释放Connection对象 
}

//处理客户端的请求报文，在Connection类中回调此函数
void TcpServer::handleMessage(s_ptrConnection conn, std::string &message)
{
    // printf("TcpServer::handleMessage:%s\n", message.data());
    if(m_handleMessageCallback) m_handleMessageCallback(conn, message);
}

void TcpServer::sendComplete(s_ptrConnection conn) // send发送成功后的回调函数，在Connection类中回调此函数
{
    if(m_sendCompleteCallback) m_sendCompleteCallback(conn);
}

void TcpServer::epollTimeout(EventLoop *loop) // epoll_wait超时，在EventLoop类中回调此函数
{
    if (m_epollTimeout)
        m_epollTimeout(loop);
}

void TcpServer::setNewConnection(std::function<void(s_ptrConnection)> fn)
{
        m_newConnectionCallback = fn;
}
void TcpServer::setCloseConnection(std::function<void(s_ptrConnection)> fn)
{
        m_closeConnectionCallback = fn;
}
void TcpServer::setErrorConnection(std::function<void(s_ptrConnection)> fn)
{
        m_errorConnectionCallback = fn;
}
void TcpServer::setHandleMessage(std::function<void(s_ptrConnection, std::string &)> fn)
{
        m_handleMessageCallback = fn;
}
void TcpServer::setSendComplete(std::function<void(s_ptrConnection)> fn)
{
    m_sendCompleteCallback = fn;
}
void TcpServer::setEpollTimeout(std::function<void(EventLoop * )> fn)
{
    m_epollTimeout = fn;
}

// 删除m_ConnectionMap中的Connection对象，在EventLoop::handleTimer()中将回调此函数
void TcpServer::timeoutRemoveConnection(int fd)
{
    {
        std::lock_guard<std::mutex> gd(m_connMapMutex);
        m_ConnectionMap.erase(fd);
    }
    if(m_handleConnectionTimeoutCallback){
        m_handleConnectionTimeoutCallback;
    }
}
//处理Connection连接超时
void TcpServer::setHandleConnectionTimeout(std::function<void(int)> fn)
{
    m_handleConnectionTimeoutCallback = fn;
}
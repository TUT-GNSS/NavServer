#include "Connection.h"

Connection::Connection(EventLoop *loop, std::unique_ptr<Socket> clientSock) : m_loop(loop), m_clientSock(std::move(clientSock)), m_disconnect(false), m_clientChannel(new Channel(m_loop, m_clientSock->fd()))
{
    // 为新客户端连接准备读事件，并添加到epoll中。
    m_clientChannel->setReadCallback(std::bind(&Connection::handleMessage, this));
    m_clientChannel->setCloseCallback(std::bind(&Connection::closeCallback, this));
    m_clientChannel->setErrorCallback(std::bind(&Connection::errorCallback, this));
    m_clientChannel->setWriteCallback(std::bind(&Connection::writeCallback, this));
    m_clientChannel->useEt();         // 客户端连接上来的fd采用边缘触发
    m_clientChannel->enableReading(); // 让epoll_wait()监视m_clientChannel的读事件
}

Connection::~Connection()
{
    // delete m_clientSock;
    // delete m_clientChannel;
    // printf("conn已经析构\n");
}

int Connection::fd() const
{
    return m_clientSock->fd();
}
std::string Connection::ip() const
{
    return m_clientSock->ip();
}
uint16_t Connection::port() const
{
    return m_clientSock->port();
}

void Connection::closeCallback() // Tcp连接关闭（断开）的回调函数，供Channel回调
{
    m_disconnect = true;
    m_clientChannel->removeChannel();//从事件循环中删除Channel
    m_closeCallback(shared_from_this());
}
void Connection::errorCallback() // Tcp连接错误的回调函数，供Channel回调
{
    m_disconnect = true;
    m_clientChannel->removeChannel();//从事件循环中删除Channel
    m_errorCallback(shared_from_this());
}


void Connection::writeCallback()
{
    // printf("Connection::writeCallback() thread id is %ld.\n", syscall(SYS_gettid));
    // std::cout << "send前:" << std::string(m_outputBuffer.data(),m_outputBuffer.size()) << std::endl;
    // printf("send():%s,(%ld)\n", m_outputBuffer.data(),m_outputBuffer.size());
    
    int writeSize = ::send(fd(), m_outputBuffer.data(), m_outputBuffer.size(), 0); //;尝试将m_onputBuffer中的数据全部发出去
    // std::cout << "send后:" << std::string(m_outputBuffer.data(), m_outputBuffer.size()) << std::endl;
    if (writeSize == -1)
    {
        perror("send()错误,Connction::writeCallback\n");
        exit(-1);
    }
    if (writeSize > 0)
    {
        m_outputBuffer.erase(0,writeSize);//从m_outputBuffer中删除已发送的数据
    }
    if(m_outputBuffer.size()==0){
        //如果发送缓冲区中没有数据了，表示已经完成发送，不需要再关注写事件
        m_clientChannel->disableWriting();
        m_sendCompleteCallback(shared_from_this());
    }
}
// 设置关闭m_fd的回调函数
void Connection::setCloseCallback(std::function<void(s_ptrConnection)> fn)
{
    m_closeCallback = fn;
}
// 设置m_fd发生错误的回调函数
void Connection::setErrorCallback(std::function<void(s_ptrConnection)> fn)
{
    m_errorCallback = fn;
}

void Connection::handleMessage() // 处理对端发送过来的消息
{
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(fd(), buffer, sizeof(buffer));
        if (nread > 0) // 成功的读取到了数据。
        {
            m_inputBuffer.append(buffer, nread); // 把读取的数据存到接收缓冲区
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {
            continue;
        }
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            std::string message;
            while (true)
            {
                if(m_inputBuffer.pickMessage(message)==false){
                    break;
                }
                // printf("message(eventfd=%d):%s\n", fd(), message.data());

                m_lastTime = TimeStamp::now();  //更新Connection时间戳
                // std::cout << "lastTime=" << m_lastTime.toString() << std::endl;

                m_handleMessageCallback(shared_from_this(), message); // 回调TcpServer::handleMessage
            }
            break;//!!!!!!!!!!!!!!!!!!!!!!!
        }
        else if (nread == 0) // 客户端连接已断开。
        {
            closeCallback(); // 调用m_closeCallback回调函数
            break;
        }
    }
}

void Connection::setHandleMessageCallback(std::function<void(s_ptrConnection, std::string &)> fn)
{
    m_handleMessageCallback = fn;
}

void Connection::setSendCompleteCallback(std::function<void(s_ptrConnection)> fn)
{
    m_sendCompleteCallback = fn;
}

// 发送数据,不管在任何线程中，都调用此函数发送数据
void Connection::sendMessage(const char *data, size_t sz)
{
    if(m_disconnect==true){
        printf("客户端连接断开,send()直接返回.\n");
        return;
    }
    //判断当前线程是否为事件循环线程(IO)
    if(m_loop->isInLoopThread()){
        //如果当前线程是IO线程，直接调用sendInLoop()发送数据
        // printf("send()在事件循环中\n");
        sendInLoop(data, sz);
    }
    else
    {
        //如果当前线程不是IO线程，调用EventLoop::addTaskToQueueInLoop(),把sendInLoop()交给事件循环线程执行
        // printf("send()不在事件循环中\n");
        m_loop->addTaskToQueueInLoop(std::bind(&Connection::sendInLoop, this, data, sz));
    }
}

// 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将此函数传给IO线程
void Connection::sendInLoop(const char *data, size_t sz)
{
    m_outputBuffer.appendWithSeparator(data, sz); // 将要发送的数据保存在Connection的接收缓冲区m_outputBuffer中
    m_clientChannel->enableWriting(); // 注册写事件
}

// 判断当前时间是否超时(空闲太长时间)
bool Connection::hasTimeout(time_t now,int interval)
{
    return now - m_lastTime.toInt() > interval;
}
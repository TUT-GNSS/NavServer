#include "Navigation.h"

Navigation::Navigation(const std::string &ip, const uint16_t port,  int subThreadNum, int workThreadNum) : m_tcpServer(ip, port, subThreadNum),m_workThreadPool(workThreadNum,"WORKS")
{
    m_tcpServer.setNewConnection(std::bind(&Navigation::handleNewConnection, this, std::placeholders::_1));
    m_tcpServer.setCloseConnection(std::bind(&Navigation::handleCloseConnection, this, std::placeholders::_1));
    m_tcpServer.setErrorConnection(std::bind(&Navigation::handleErrorConnection, this, std::placeholders::_1));
    m_tcpServer.setHandleMessage(std::bind(&Navigation::handleMessage, this, std::placeholders::_1, std::placeholders::_2));
    m_tcpServer.setSendComplete(std::bind(&Navigation::handleSendComplete, this, std::placeholders::_1));
    // m_tcpServer.setEpollTimeout(std::bind(&Navigation::handleEpollTimeout, this, std::placeholders::_1));
    m_tcpServer.setHandleConnectionTimeout(std::bind(&Navigation::handleConnectionTimeout, this, std::placeholders::_1));
}
Navigation::~Navigation()
{
}

void Navigation::startLoopRun()
{
    m_tcpServer.startLoopRun();
}

void Navigation::stopLoopRun()
{
    //停止工作线程
    m_workThreadPool.stopThread();
    printf("工作线程已经停止！\n");
    //停止IO线程(事件循环)
    m_tcpServer.stopLoopRun();
}

void Navigation::handleNewConnection(s_ptrConnection conn) // 处理新客户端连接请求，在TcpServer类中回调此函数
{
    //新设备连接，将设备信息保存到状态机中
    spDeviceInfo deviceIfo(new DeviceInfo(conn->fd(),conn->ip()));
    {
        std::lock_guard<std::mutex> gd(m_deviceMutex);
        m_deviceInfoMap[conn->fd()] = deviceIfo;    //把客户端设备加入状态机中
    }
    printf("%s 新建立连接(ip=%s).\n", TimeStamp::now().toString().data(), conn->ip().data());

}

void Navigation::handleCloseConnection(s_ptrConnection conn) // 关闭客户端的连接，在TcpServer类中回调此函数
{
    printf("%s 设备断开连接(ip=%s).\n", TimeStamp::now().toString().data(), conn->ip().data());
    //关闭客户端连接的时候，从客户端删除连接信息
    {
        std::lock_guard<std::mutex> gd(m_deviceMutex);
        m_deviceInfoMap.erase(conn->fd());  //从状态机删除设备信息
    }
}

void Navigation::handleErrorConnection(s_ptrConnection conn) // 客户端连接错误，在TcpServer类中回调此函数
{
    handleCloseConnection(conn);
}
void Navigation::handleMessage(s_ptrConnection conn, std::string &message) // 处理客户端的请求报文，在TcpServer类中回调此函数
{
    if(m_workThreadPool.size()==0){
        //如果没有工作线程，表示在IO线程中计算
        handleMessageToThreadPool(conn, message);
    }
    else{
        // 把业务添加到工作线程池的任务队列中,交给工作线程去处理业务
        m_workThreadPool.addTask(std::bind(&Navigation::handleMessageToThreadPool, this, conn, message));
    }
}
void Navigation::handleSendComplete(s_ptrConnection conn) // send发送成功后的回调函数，在TcpServer类中回调此函数
{
    // std::cout << " Message send complete." << std::endl;
    // 增加其它代码
}
// void Navigation::handleEpollTimeout(EventLoop *loop) // epoll_wait超时，在TcpServer类中回调此函数
// {
//     // std::cout << "Navigation timeout " << std::endl;
//     // 增加其它代码
// }

//处理XML报文函数
bool getXMLBuffer(const std::string &xmlBuffer, const std::string &fieldName, std::string &value, const int infoLen = 0)
{
    std::string start = "<" + fieldName + ">"; // 数据项开始标签
    std::string end = "</" + fieldName + ">";  // 数据项结束标签

    int startPos = xmlBuffer.find(start); // 在xml中查找数据项开始的标签位置
    if (startPos == std::string::npos)
    {
        return false;
    }

    int endPos = xmlBuffer.find(end); // 在xml中查找数据结束的标签位置
    if (endPos == std::string::npos)
    {
        return false;
    }

    // 从xml中截取数据项内容
    int infoTmpLen = endPos - startPos - start.length();
    if ((infoLen > 0) && (infoLen < infoTmpLen))
    {
        infoTmpLen = infoLen;
    }
    // printf("infoTmpLen:%d\n", infoTmpLen);
    value = xmlBuffer.substr(startPos + start.length(), infoTmpLen);
    // std::cout << value << std::endl;
    return true;
}

// 处理客户端的请求报文,用于添加给线程池
void Navigation::handleMessageToThreadPool(s_ptrConnection conn, std::string &message)
{
    // printf("Navigation::handleMessageToThreadPool!");
    spDeviceInfo deviceInfo = m_deviceInfoMap[conn->fd()]; // 从状态机中获取客户端信息
    // 从请求报文中解析出请求报文
    // 后续改为<bizcode>00101</bizcode><username>liujixi<username><password>123456</password>
    // 暂时为<bizcode>00101</bizcode><deviceid>001<deviceid>
    // 暂时为<bizcode>00201</bizcode><navdata>... ... ...<navdata>
    std::string bizcode; //业务代码
    std::string replayMessage;//回应报文
    if((getXMLBuffer(message, "bizcode", bizcode))==false)//从请求报文中解析业务代码
    {
        // printf("发送端bizcode报文错误");
        bizcode="00000";
    }
    if(bizcode=="00101") //登录业务
    {
        std::string deviceid;

        if ((getXMLBuffer(message, "deviceid", deviceid)) == false) // 解析设备id
        {
            // printf("发送端deviceid报文错误");
            deviceid = "-1";
        }

        if(deviceid!="-1"){
            replayMessage = "<bizcode>00102</bizcode><retcode>0</retcode><message>ok</message>";
            // 初始化客户端设备状态机
            if ((deviceInfo->deviceInit(deviceid)) == false)
            {
                perror("客户端设备状态机初始化失败！");
                exit(-1);
            }
        }
        else{
            //发送端报文错误
            replayMessage = "<bizcode>00102</bizcode><retcode>-1</retcode><message>报文格式错误</message>";
        }
    }
    else if(bizcode=="00201")//导航GNSS数据输出到文件中
    {
        if(deviceInfo->hasLogin()){
            std::string gnssdata;
            getXMLBuffer(message, "gnssdata", gnssdata);//解析设备导航数据

            deviceInfo->outputGnssData(gnssdata); // 导航数据输出到文件
            replayMessage = "<bizcode>00209</bizcode><retcode>0</retcode><message>ok</message>";
        }
        else{
            replayMessage = "<bizcode>00209</bizcode><retcode>-1</retcode><message>设备未登录</message>";
        }
    }
    else if (bizcode == "00202") // 导航IMU数据输出到文件中
    {
        if (deviceInfo->hasLogin())
        {
            std::string imudata;
            getXMLBuffer(message, "imudata", imudata);     // 解析设备导航数据

            deviceInfo->outputImuData(imudata);            // 导航数据输出到文件
            replayMessage = "<bizcode>00209</bizcode><retcode>0</retcode><message>ok</message>";
        }
        else
        {
            replayMessage = "<bizcode>00209</bizcode><retcode>-1</retcode><message>设备未登录</message>";
        }
    }
    else if(bizcode=="00901")//注销业务
    {
        if(deviceInfo->hasLogin()){
            deviceInfo->deviceClose();//注销状态机
            replayMessage = "<bizcode>00902</bizcode><retcode>0</retcode><message>ok</message>";
        }
        else{
            replayMessage = "<bizcode>00902</bizcode><retcode>-1</retcode><message>设备未登录</message>";
        }
    }
    conn->sendMessage(replayMessage.data(), replayMessage.size());
}

void Navigation::handleConnectionTimeout(int fd) // 客户端连接超时，在TcpServer类中回调此函数
{
    printf("fd(%d)已超时！", fd);
    std::lock_guard<std::mutex>gd(m_deviceMutex);
    m_deviceInfoMap.erase(fd);
}

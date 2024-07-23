#pragma once
#include "TcpServer.h"
#include "EventLoop.h"
#include "Connection.h"
#include <fstream>

/*
    Navigation类：组合导航信息采集服务器
*/


class DeviceInfo        //客户端设备的信息(状态机)//传入客户端连接的Connection的fd和ip，接收登录信息时初始化
{
    private:
        int m_deviceFD;             // 客户端设备fd
        std::string m_deviceIP;     //  客户端设备的ip地址
        bool m_deviceLogin = false; // 客户端设备登录的状态 true登录 false 未登录
        std::string m_deviceID;     //客户端设备的id
        std::ofstream m_deviceGnssDataOfstream;  // 客户端设备导航数据输出数据流
        std::ofstream m_deviceImuDataOfstream;  // 客户端设备导航数据输出数据流

    public:
        DeviceInfo(int fd, const std::string &ip) : m_deviceFD(fd), m_deviceIP(ip) {}
        ~DeviceInfo() { deviceClose(); }
        // void setLogin(bool login) { m_deviceLogin = login; }
        bool hasLogin() { return m_deviceLogin; }
        bool deviceInit(const std::string &deviceID) { 
            m_deviceID = deviceID;
            m_deviceGnssDataOfstream.open("../NavData/GnssData/device" + deviceID + "_gnss.txt", std::ios::out | std::ios::trunc); // 设置输出流路径
            m_deviceImuDataOfstream.open("../NavData/ImuData/device" + deviceID + "_imu.txt", std::ios::out | std::ios::trunc);  // 设置输出流路径
            if ((!m_deviceGnssDataOfstream.is_open()) || (!m_deviceImuDataOfstream.is_open()))
            {
                return false;
            }
            m_deviceLogin=true;
            return true;
        }

        void deviceClose(){
            m_deviceGnssDataOfstream.close();
            m_deviceImuDataOfstream.close();
            m_deviceID.clear();
            m_deviceLogin = false;
        }

        void outputGnssData(const std::string &data){
            // printf("deviceID:%s\n", m_deviceID.data());
            // printf("outputData:%s,(%ld) \n", data.data(),data.size());
            m_deviceGnssDataOfstream << data << "\n";
        }

        void outputImuData(const std::string &data)
        {
            // printf("deviceID:%s\n", m_deviceID.data());
            // printf("outputData:%s,(%ld) \n", data.data(),data.size());
            m_deviceImuDataOfstream << data << "\n";
        }
};

class Navigation
{
private:
    using spDeviceInfo = std::shared_ptr<DeviceInfo>;//给设备客户端状态机定义别名

    TcpServer m_tcpServer;
    ThreadPool m_workThreadPool;    //工作线程池
    std::string m_deviceName;
    std::mutex m_deviceMutex; // 保护m_deviceInfoMap的互斥锁
    std::map<int, spDeviceInfo> m_deviceInfoMap;//存放设备客户端状态机

public:
    Navigation(const std::string &ip,const uint16_t port,int subThreadNum=3,int workThreadNum=5);
    ~Navigation();

    void startLoopRun();    //启动服务
    void stopLoopRun();    //停止服务

    void handleNewConnection(s_ptrConnection conn); // 处理新客户端连接请求，在TcpServer类中回调此函数
    void handleCloseConnection(s_ptrConnection conn); // 关闭客户端的连接，在TcpServer类中回调此函数
    void handleErrorConnection(s_ptrConnection conn); // 客户端连接错误，在TcpServer类中回调此函数
    void handleMessage(s_ptrConnection conn, std::string &message); // 处理客户端的请求报文，在TcpServer类中回调此函数
    void handleSendComplete(s_ptrConnection conn);                   // send发送成功后的回调函数，在TcpServer类中回调此函数
    // void handleEpollTimeout(EventLoop *loop);      // epoll_wait超时，在TcpServer类中回调此函数

    void handleMessageToThreadPool(s_ptrConnection conn, std::string &message); // 处理客户端的请求报文,用于添加给线程池
    void handleConnectionTimeout(int fd);       //客户端连接超时，在TcpServer类中回调此函数
};
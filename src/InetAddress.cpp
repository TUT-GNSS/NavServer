#include "InetAddress.h"

InetAddress::InetAddress(){

}

InetAddress::InetAddress(const std::string &ip, uint16_t port){      // 如果是监听的fd，用这个构造函数。
    m_addr.sin_family = AF_INET;                                 // IPv4网络协议的套接字类型。
    m_addr.sin_port = htons(port);                               // 服务端用于监听的ip地址。
    m_addr.sin_addr.s_addr= inet_addr(ip.data());        // 服务端用于监听的端口。
}
InetAddress::InetAddress(const sockaddr_in addr):m_addr(addr){   // 如果是客户端连上来的fd，用这个构造函数。

}

InetAddress::~InetAddress(){

}

const char *InetAddress::ip() const{ // 返回字符串表示的地址，例如：192.168.150.128
    return inet_ntoa(m_addr.sin_addr);
}
uint16_t InetAddress::port() const{ // 返回整数表示的端口，例如：80、8080
    return ntohs(m_addr.sin_port);
}
const sockaddr *InetAddress::addr() const{// 返回addr_成员的地址，转换成了sockaddr。
    return (sockaddr *)&m_addr;
}

void InetAddress::setAddr(sockaddr_in clientAddr){
    m_addr = clientAddr;
}
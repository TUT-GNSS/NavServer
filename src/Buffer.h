#pragma once
#include <iostream>
#include <string>
#include <cstring>

class Buffer
{
private:
    std::string m_buffer;       // 用于存放数据
    const uint16_t m_separator; // 报文的分隔符：0-无分隔符；(固定长度、视频会议) 1-四字节的报头；2-"\r\n\r\n"分隔符（http协议）

public:
    Buffer(uint16_t separator =1);
    ~Buffer();

    void append(const char *data, size_t size);              // 把数据追加到m_buffer中
    void appendWithSeparator(const char *data, size_t size); // 把数据追加到m_buffer中，附加报文头部
    size_t size();                                           // 返回m_buffer的大小
    const char *data();                                      // 返回m_buffer的首地址
    void clear();                                            // 清空m_buffer
    void erase(size_t pos, size_t n);                        // 从m_buffer中删除，从位置pos开始删除n个字节

    bool pickMessage(std::string &str); // 从m_buffer中产分一个报文存在str中，没有返回false
};

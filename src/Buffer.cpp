#include "Buffer.h"

Buffer::Buffer(uint16_t separator):m_separator(separator)
{

}
Buffer::~Buffer(){

}

// 把数据追加到m_buffer中
void Buffer::append(const char *data, size_t size)
{
    m_buffer.append(data, size);
}

// 返回m_buffer的大小
size_t Buffer::size()                             
{
    return m_buffer.size();
}

// 返回m_buffer的首地址
const char *Buffer::data()                        
{
    return m_buffer.data();
}

// 清空m_buffer
void Buffer::clear()                             
{
    m_buffer.clear();
}

// 从m_buffer中删除，从位置pos开始删除n个字节
void Buffer::erase(size_t pos, size_t n){
    m_buffer.erase(pos, n);
}

void Buffer::appendWithSeparator(const char *data, size_t size)
{
    if(m_separator==0){
        m_buffer.append(data, size);
    }
    else if(m_separator==1){
        m_buffer.append((char *)&size, 4);
        m_buffer.append(data, size);
    }
    else if(m_separator==2){
        printf("Http分隔符,还没写");
        exit(0);

    }
    else{
        printf("无效的分隔符！");
        exit(-1);
    }
}

// 从m_buffer中产分一个报文存在str中，没有返回false
bool Buffer::pickMessage(std::string &str)
{
    if(m_buffer.empty()){
        return false;
    }

    if(m_separator==0){
        str = m_buffer;
        m_buffer.clear();
    }
    else if(m_separator==1){
        int len;
        memcpy(&len, m_buffer.data(), 4); // 获取m_Buffer的报文头部 长度4字节
        // 如果m_Buffer的数据量小于报文头部，说明m_Buffer报文内容不完整
        if (m_buffer.size() < len + 4)
        {
            return false;
        }
        str = m_buffer.substr(4, len); // 从m_Buffer中获取一个报文
        m_buffer.erase(0, len + 4);
    }
    else if (m_separator == 2)
    {
        // int len;
        // memcpy(&len, m_buffer.data()+4, 4); // 获取m_Buffer的报文头部 长度4字节
        // // 如果m_Buffer的数据量小于报文头部，说明m_Buffer报文内容不完整
        // if (m_buffer.size() < len + 8)
        // {
        //     return false;
        // }
        // str = m_buffer.substr(8, len); // 从m_Buffer中获取一个报文
        // m_buffer.erase(0, len + 8);
    }

    return true;
}
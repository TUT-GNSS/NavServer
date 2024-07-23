#include <time.h>
#include "TimeStamp.h"

// 用当前时间初始化对象
TimeStamp::TimeStamp()                
{
    m_secondSinceEpoch = time(0);
}

// 用一个整数表示的时间初始化对象
TimeStamp::TimeStamp(int64_t secondSinceEpoch):m_secondSinceEpoch(secondSinceEpoch) 
{

}

TimeStamp::~TimeStamp()
{

}


 // 返回当前时间的TimeStamp对象
TimeStamp TimeStamp::now()
{
    return TimeStamp();
}

 // 返回整数表示的时间
time_t TimeStamp::toInt() const  
{
    return m_secondSinceEpoch;
}

// 返回字符串表示的时间，格式yyyy-mm-dd hh24:mi:ss
std::string TimeStamp::toString() const 
{
    char buf[128] = {0};
    tm *tm_time = localtime(&m_secondSinceEpoch);
    snprintf(buf, 128, "%4d-%02d-%02d %02d:%02d:%02d", tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return buf;
}

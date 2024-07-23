#pragma once

#include <iostream>
#include <string>

//时间戳
class TimeStamp
{
    private:
        time_t m_secondSinceEpoch;  //整数表示的时间(从1970到现在已逝去的秒数)
    public:
        TimeStamp();        //用当前时间初始化对象
        TimeStamp(int64_t secondSinceEpoch);    //用一个整数表示的时间初始化对象
        ~TimeStamp();

        static TimeStamp now();         //返回当前时间的TimeStamp对象
        time_t toInt() const;               //返回整数表示的时间
        std::string toString() const;   //返回字符串表示的时间，格式yyyy-mm-dd hh24:mi:ss
};
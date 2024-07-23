#include "Navigation.h"
#include <signal.h>

Navigation *navServer;

//信号处理函数
void Stop(int sig)
{
    printf("sig=%d, navServer已经停止\n", sig);
    //调用Navigation::stop()停止服务
    navServer->stopLoopRun();
    delete navServer;
    printf("delete navServer\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./tcpepoll ip port\n");
        printf("example: ./navServer 192.168.205.129 5005\n\n");
        return -1;
    }
    signal(SIGTERM, Stop);//捕获信号15
    signal(SIGINT,Stop);//捕获信号2

    navServer = new Navigation(argv[1], atoi(argv[2]), 3, 0); // 初始化Tcp客户端对象
    navServer->startLoopRun();                                // 开启Navigation的事件循环
    return 0;
}


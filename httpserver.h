#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include <string.h>


class HTTPServer
{
public:
    static const int MAX_FD = 65535;            //最大文件描述符
    static const int MAX_EVENT_NUM = 10000;  //最大事件数

    //相关配置信息
    int m_port;
    char *m_root;

    //epoll相关
    int m_epollfd;
    epoll_event events[MAX_EVENT_NUM];

    int m_listenfd;

public:
    HTTPServer();
    ~HTTPServer();

    //初始化
    void init(int port);
    //事件监听
    void eventListen();
    //循环运行
    void eventLoop();
};


#endif
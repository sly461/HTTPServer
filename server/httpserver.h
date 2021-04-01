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
#include <assert.h>
#include <sys/epoll.h>
#include <string.h>


class HTTPServer
{
public:
    HTTPServer(short port);
    ~HTTPServer();

    void Run();

private:
    static const int MAX_FD = 65536;
    static int SetFdNonblock(int fd);

    int m_port;         //端口
    char *m_rootDir;    //根目录
    int m_listenFd;     //监听fd
    bool m_isClose;

    //epoll相关
    

    bool InitSocket();
    
};


#endif
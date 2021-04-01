#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H

#include <fcntl.h>  
#include <unistd.h>      
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <string.h>
#include <unordered_map>

#include "epoller.h"

class HTTPServer
{
public:
    HTTPServer(short port, int timeout=-1);
    ~HTTPServer();

    void Run();

private:
    static const int MAX_FD = 65536;
    static int SetFdNonblock(int fd);

    int m_port;         //端口
    int m_timeout;      //epoll_wait等待时间
    char *m_rootDir;    //根目录
    int m_listenFd;     //监听fd
    bool m_isClose;

    //epoll相关
    std::unique_ptr<Epoller> m_epoller;

    bool InitSocket();
    void DoListen();
    void DoRead();
    void DoWrite();
    
};


#endif
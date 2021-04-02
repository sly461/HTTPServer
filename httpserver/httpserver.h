#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H

#include <fcntl.h>     
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>

#include "epoller.h"
#include "../http/httpconn.h"

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
    //一个socketfd对应一个HTTPConn对象
    std::unordered_map<int, HTTPConn> m_users;

    bool InitSocket();
    void DealListen();
    void DealRead(HTTPConn* conn);
    void DealWrite(HTTPConn* conn);
    void CloseConn(HTTPConn* conn);
    
};


#endif
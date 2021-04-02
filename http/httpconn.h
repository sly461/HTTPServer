/**
 * 一个http连接 包括对端地址、与对端相连的connFd、以及读写缓存区
 * 也包含一个http请求对象和http响应对象
**/
#ifndef _HTTPCONN_H_
#define _HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>   
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <errno.h>      
#include <string.h>

class HTTPConn {
public:
    HTTPConn();
    ~HTTPConn();

    //设置fd和addr 可重复使用该对象
    void Set(int socketfd, const sockaddr_in& addr);
    //Get
    int GetFd() const;
    int GetPort() const;
    const char * GetIP() const;
    sockaddr_in GetAddr() const;
    //关闭该连接
    void Close();

    static const char * ROOTDIR;
    static int userCnt;

private:
    int m_connFd;
    struct sockaddr_in m_addr;
    
    bool m_isClose;
};

#endif
/**
 * 一个http连接 包括对端地址、与对端相连的connFd、以及读写缓存区
 * 也包含一个http请求对象和http响应对象
**/
#ifndef _HTTPCONN_H_
#define _HTTPCONN_H_

#include <sys/types.h>
#include <sys/uio.h>   
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <errno.h>
#include <string.h>

#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HTTPConn {
public:
    HTTPConn();
    ~HTTPConn();

    static const char * ROOTDIR;
    static int userCnt;

    //设置fd和addr 可重复使用该对象
    void Init(int socketfd, const sockaddr_in& addr);

    //Get
    int GetFd() const;
    int GetPort() const;
    const char * GetIP() const;
    sockaddr_in GetAddr() const;

    //使用buffer read write
    ssize_t Read(int * saveErrno);
    ssize_t Write(int * saveErrno);
    //要写的字节数
    size_t ToWriteBytes();

    //是否保持长连接
    bool IsKeepAlive() const;

    //处理读取的数据
    bool Process();

    //关闭该连接
    void Close();

private:
    int m_connFd;
    struct sockaddr_in m_addr;
    
    bool m_isClose;

    struct iovec m_iov[2];
    int m_iovCnt;

    //读写缓冲区
    Buffer m_readBuffer;
    Buffer m_writeBuffer;

    //http请求与响应
    HTTPRequest m_httpRequest;
    HTTPResponse m_httpResponse;
};

#endif
#include "httpconn.h"

const char * HTTPConn::ROOTDIR;
int HTTPConn::userCnt;

HTTPConn::HTTPConn(): 
    m_connFd(-1), m_isClose(true)
{
    bzero(&m_addr, sizeof(m_addr));
}

HTTPConn::~HTTPConn() {
    Close();
}

void HTTPConn::Set(int socketFd, const sockaddr_in& addr) {
    m_connFd = socketFd;
    m_addr = addr;
    userCnt++;
    m_isClose = false;
}

int HTTPConn::GetFd() const {
    return m_connFd;
}

int HTTPConn::GetPort() const {
    return ntohs(m_addr.sin_port);
}
    
const char * HTTPConn::GetIP() const {
    return inet_ntoa(m_addr.sin_addr);
}
    
sockaddr_in HTTPConn::GetAddr() const {
    return m_addr;
}

void HTTPConn::Close() {
    if(!m_isClose) {
        close(m_connFd);
        userCnt --;
        bzero(&m_addr, sizeof(m_addr));
        m_isClose = true;
    }
}
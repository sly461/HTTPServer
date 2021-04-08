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
    m_httpResponse.UnmapFile();
    if(!m_isClose) {
        close(m_connFd);
        userCnt --;
        bzero(&m_addr, sizeof(m_addr));
        m_isClose = true;
    }
}

ssize_t HTTPConn::Read(int * saveErrno) {
    ssize_t len = -1;
    while(1) {
        char buf[65535];
        size_t writable = m_readBuffer.WritableBytes();
        m_iov[0].iov_base = m_readBuffer.BeginWritePtr();
        m_iov[0].iov_len = writable;
        m_iov[1].iov_base = buf;
        m_iov[1].iov_len = sizeof(buf);
        m_iovCnt = 2;
        
        len = readv(m_connFd, m_iov, m_iovCnt);
        if(len == 0) break;
        if(len < 0) {
            *saveErrno = errno;
            break;
        }
        else if(static_cast<size_t>(len) <= writable)
            m_readBuffer.HasWritten(static_cast<size_t>(len));
        else {
            //readbuffer写满了
            m_readBuffer.HasWritten(writable);
            //将buf里的内容转移到readbuffer
            m_readBuffer.Append(buf, static_cast<size_t>(len)-writable);
        }
    }
    return len;
}
    
ssize_t HTTPConn::Write(int * saveErrno) {
    
}

bool HTTPConn::Process() {
    //没有可读的数据
    if(m_readBuffer.ReadableBytes() <= 0) return false;

    //request
    m_httpRequest.Init();
    if(m_httpRequest.Parse(m_readBuffer)) {
        //std::cout << m_httpRequest.GetMethod() << " " << m_httpRequest.GetPath() <<  " " <<m_httpRequest.GetVersion() << std::endl;
    }
    else {

    }

    //response

    return true;
}
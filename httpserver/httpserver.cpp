#include "httpserver.h"


HTTPServer::HTTPServer(short port, int timeout, int threadCnt): 
    m_port(port), m_isClose(false), m_timeout(timeout), m_threadPool(threadCnt)
{
    //获取根目录
    m_rootDir = getcwd(nullptr, 256);
    strcat(m_rootDir, "/root/");
    //HTTPConn初始化
    HTTPConn::ROOTDIR = m_rootDir;
    HTTPConn::userCnt = 0;
    //初始化listenFd
    m_isClose = !InitSocket();
}

HTTPServer::~HTTPServer() {
    close(m_listenFd);
    m_isClose = true;
    free(m_rootDir);
}

bool HTTPServer::InitSocket() {
    //服务器地址与端口
    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //监听套接字
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(m_listenFd>0);
    //端口复用
    int opt = 1;
    int ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    assert(ret!=-1);
    //设置非阻塞
    ret = SetFdNonblock(m_listenFd);
    assert(ret!=-1);
    //bind绑定
    ret = bind(m_listenFd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret >= 0);
    //设置监听上限
    ret = listen(m_listenFd, 6);
    assert(ret >= 0);
    //设置事件到EPOLL上 ET
    m_epoller.OperateFd(EPOLL_CTL_ADD, m_listenFd, EPOLLRDHUP|EPOLLET|EPOLLIN);
    
    return true;
}

void HTTPServer::OnRead(HTTPConn* conn) {
    int readErrno = 0;
    ssize_t ret = conn->Read(&readErrno);
    if(ret <= 0 && readErrno!=EAGAIN) {
        //关闭socket
        CloseConn(conn);
        return;
    }
    OnProcess(conn);
}

void HTTPServer::OnProcess(HTTPConn* conn) {
    //有数据且已将其处理完 挂写事件
    if(conn->Process())
        m_epoller.OperateFd(EPOLL_CTL_MOD ,conn->GetFd(), EPOLLRDHUP|EPOLLONESHOT|EPOLLET|EPOLLOUT);
    //没有读到数据 继续读 
    else
        m_epoller.OperateFd(EPOLL_CTL_MOD ,conn->GetFd(), EPOLLRDHUP|EPOLLONESHOT|EPOLLET|EPOLLIN);
}
    
void HTTPServer::OnWrite(HTTPConn* conn) {
    int writeErrno = 0;
    ssize_t ret = conn->Write(&writeErrno);
    //传输完成
    if(conn->ToWriteBytes() == 0) {
        if(conn->IsKeepAlive()) {
            //保持长连接 则继续IN事件
            m_epoller.OperateFd(EPOLL_CTL_MOD, conn->GetFd(), EPOLLRDHUP|EPOLLONESHOT|EPOLLET|EPOLLIN);
            return;
        }
    }
    else if(ret < 0) {
        //继续传输
        if(writeErrno == EAGAIN) {
            m_epoller.OperateFd(EPOLL_CTL_MOD, conn->GetFd(), EPOLLRDHUP|EPOLLONESHOT|EPOLLET|EPOLLOUT);
            return;
        }
    }
    CloseConn(conn);
}

void HTTPServer::DealListen() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    //ET模式 必须一次性收完
    do {
        int connFd = accept(m_listenFd, (sockaddr *)&clientAddr, &clientAddrLen);
        if(connFd <= 0) break;
        else if(HTTPConn::userCnt >= MAX_FD) {
            //TODO "Server busy!"
            break;
        }
        //ET模式 设置非阻塞
        SetFdNonblock(connFd);
        //添加事件到epoll树上
        //只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket，
        //需要再次把这个socket加入到EPOLL队列里
        m_epoller.OperateFd(EPOLL_CTL_ADD, connFd, EPOLLRDHUP|EPOLLONESHOT|EPOLLET|EPOLLIN);
        //添加客户端到m_users
        m_users[connFd].Init(connFd, clientAddr);

    } while(1);
}

void HTTPServer::DealRead(HTTPConn* conn) {
    //添加到线程池
    m_threadPool.AddTask(std::bind(&HTTPServer::OnRead, this, conn));
}

void HTTPServer::DealWrite(HTTPConn* conn) {
    //添加到线程池
    m_threadPool.AddTask(std::bind(&HTTPServer::OnWrite, this, conn));
}

void HTTPServer::CloseConn(HTTPConn* conn) {
    //从epoll树上摘除
    m_epoller.OperateFd(EPOLL_CTL_DEL, conn->GetFd(), 0);
    //关闭fd 关闭连接
    conn->Close();
}

void HTTPServer::Run() {
    while(!m_isClose) {
        int eventCnt = m_epoller.Wait(m_timeout);
        for(int i=0; i<eventCnt; i++) {
            //处理事件
            uint32_t events = m_epoller.GetEvent(i);
            int fd = m_epoller.GetEventFd(i);
            if(fd == m_listenFd) {
                DealListen();
            }
            //CloseConn
            //EPOLLHUP 和 EPOLLERR事件不用添加监听 会自动加上
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                CloseConn(&m_users[fd]);
            }
            //DealRead
            else if(events & EPOLLIN) {
                DealRead(&m_users[fd]);
            }
            //DealWrite
            else if(events & EPOLLOUT) {
                DealWrite(&m_users[fd]);
            }
        }
    }
}

int HTTPServer::SetFdNonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}
#include "httpserver.h"


HTTPServer::HTTPServer(short port, int timeout=-1): 
    m_port(port), m_isClose(false), m_timeout(timeout)
{
    //获取根目录
    m_rootDir = getcwd(nullptr, 256);
    strcat(m_rootDir, "/root/");
    printf("%s", m_rootDir);
    //初始化listenFd
    m_isClose = InitSocket();
}

HTTPServer::~HTTPServer() {
    close(m_listenFd);
    m_isClose = false;
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
    m_epoller->OperateFd(EPOLL_CTL_ADD, m_listenFd, EPOLLRDHUP|EPOLLET|EPOLLIN);
    
    return true;
}

void HTTPServer::Run() {
    while(!m_isClose) {
        int eventCnt = m_epoller->Wait(m_timeout);
        for(int i=0; i<eventCnt; i++) {
            //处理事件
            uint32_t events = m_epoller->GetEvent(i);
            int fd = m_epoller->GetEventFd(i);
            if(fd == m_listenFd) {
                DoListen();
            }
            //CloseConn
            //EPOLLHUP 和 EPOLLERR事件不用添加监听 会自动加上
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {

            }
            //DoRead
            else if(events & EPOLLIN) {

            }
            //DoWrite
            else if(events & EPOLLOUT) {

            }
        }
    }
}

int HTTPServer::SetFdNonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}
#include "httpserver.h"


HTTPServer::HTTPServer(short port): m_port(port), m_isClose(false)
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
    //监听套接字
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    assert(m_listenFd>0);
    //设置非阻塞
    int ret = SetFdNonblock(m_listenFd);
    assert(ret!=-1);
    //端口复用
    int opt = 1;
    ret = setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    assert(ret!=-1);
    
    //服务器地址与端口
    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind绑定
    ret = bind(m_listenFd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret >= 0);
    //设置监听上限
    ret = listen(m_listenFd, 6);
    assert(ret >= 0);

    //epoll
    //m_epollFd = epoll_create(5);
    //assert(m_epollfd != -1);
    //初始化对应的事件结构体 一般listenfd放到结构体数组的末尾
    //eventset(&g_events[MAX_EVENTS], listenfd, acceptConn, &g_events[MAX_EVENTS]);
    //添加到红黑树上
    //eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);
    return true;
}

void HTTPServer::Run() {

}

int HTTPServer::SetFdNonblock(int fd) {
    int flag = fcntl(fd, F_GETFL);
    flag |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, flag);
}
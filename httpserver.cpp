#include "httpserver.h"


void HTTPServer::init(int port) {
    m_port = port;
}

void HTTPServer::eventListen() {
    //监听套接字
    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //设置非阻塞
    // int flag = fcntl(listenfd, F_GETFL);
    // flag |= O_NONBLOCK;
    // fcntl(listenfd, F_SETFL, flag);
    //端口复用
    int opt = 1;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    //服务器地址与端口
    sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(m_port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //bind绑定
    int ret = bind(m_listenfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret >= 0);
    //设置监听上限
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    //epoll
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);
    //初始化对应的事件结构体 一般listenfd放到结构体数组的末尾
    //eventset(&g_events[MAX_EVENTS], listenfd, acceptConn, &g_events[MAX_EVENTS]);
    //添加到红黑树上
    //eventadd(g_efd, EPOLLIN, &g_events[MAX_EVENTS]);
}

void HTTPServer::eventLoop() {

}
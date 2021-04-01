/**
 * 封装epoll的基本操作
**/

#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <sys/epoll.h>
#include <fcntl.h> 
#include <unistd.h> 
#include <assert.h> 
#include <vector>
#include <errno.h>

class Epoller {
public:
    Epoller(int maxEvent = 1024);
    ~Epoller();

    //操作fd 
    //包含添加fd和监听事件到epoll红黑树上、修改fd事件、从epoll上移除fd
    bool OperateFd(int op, int fd, uint32_t events);
    //调用epoll_wait
    int Wait(int timeout);

    //根据idx得到发生事件的fd和相应的事件
    uint32_t GetEvent(int idx) const;
    int GetEventFd(int idx) const;

private:
    int m_epollFd;
    std::vector<struct epoll_event> m_events;
};

#endif
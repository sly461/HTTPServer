#include "epoller.h"


Epoller::Epoller(int maxEvent): m_epollFd(epoll_create(512)), m_events(maxEvent)
{
    assert(m_epollFd >=0 && m_events.size()>0);
}

Epoller::~Epoller() {
    close(m_epollFd);
}

bool Epoller::OperateFd(int op, int fd, uint32_t events) {
    if(op == EPOLL_CTL_DEL) 
        return 0 == epoll_ctl(m_epollFd, op, fd, nullptr);
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epollFd, op, fd, &ev);
}

int Epoller::Wait(int timeout=5000) {
    return epoll_wait(m_epollFd, &m_events[0], m_events.size(), timeout);
}

uint32_t Epoller::GetEvent(int idx) const {
    return m_events[idx].events;
}

int Epoller::GetEventFd(int idx) const {
    return m_events[idx].data.fd;
}


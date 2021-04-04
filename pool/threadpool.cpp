#include "threadpool.h"


ThreadPool::ThreadPool(size_t threadCnt): m_isClose(false) {
    for(size_t i=0; i<threadCnt; i++) {
        m_works.emplace_back([this]() {
            while(1) {
                std::function<void()> task;
                //大括号作用：临时变量的生存期，即lock出大括号则失效
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait(lock, [this]() {
                       return m_isClose || !m_tasks.empty(); 
                    });
                    //同时满足才退出
                    if(m_isClose && m_tasks.empty()) return;
                    //再次确认队列不为空
                    if(!m_tasks.empty()) {
                        task = std::move(m_tasks.front());
                        m_tasks.pop();
                    }
                }
                //执行
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    //此处也要大括号 m_tasks和m_isClose都是共享资源
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_isClose = true;
    }
    m_condition.notify_all();
    for(auto &t: m_works) t.join();
}

void ThreadPool::AddTask(std::function<void()>&& task) {
    m_mutex.lock();
    m_tasks.push(task);
    m_mutex.unlock();
    m_condition.notify_one();
}
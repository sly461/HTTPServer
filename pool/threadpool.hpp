/**
 * 线程池
**/ 
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    //默认8个线程
    ThreadPool(size_t threadCnt=8);
    ~ThreadPool();

    void AddTask(std::function<void()>&& task);
    
private:
    std::vector<std::thread> m_works;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    //关闭添加task 但是注意 队列中剩余的task需要被执行完
    bool m_isClose;
};


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


#endif
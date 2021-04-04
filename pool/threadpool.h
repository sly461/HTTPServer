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


#endif
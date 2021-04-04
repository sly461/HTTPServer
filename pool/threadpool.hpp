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

    void AddTask(std::function<void()> task);
    
private:
    std::vector<std::function<void()>> m_works;
    std::queue<std::thread> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    //关闭添加task 队列中剩余的task要执行完
    bool m_isClose;
};


ThreadPool::ThreadPool(size_t threadCnt): m_isClose(false) {

}

ThreadPool::~ThreadPool() {
    
}




#endif
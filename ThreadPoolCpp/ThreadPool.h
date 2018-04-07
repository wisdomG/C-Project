#ifdef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <funtion>
#include <mutex>
#include <condition_variable>
using namespace std;

class ThreadPool {
    public:
        ThreadPool(size_t threads);
        template<typename F, typename... Args>
        auto enqueue(F&& f, Args&&... args)
        -> future<typename result_of<F(Arg...)>::type;
        ~ThreadPool();
    private:
        vector<thread> workers;
        queue<function<void()>> tasks;
        mutex queue_mutex;
        condition_variable condition;
        bool stop;
}

#endif

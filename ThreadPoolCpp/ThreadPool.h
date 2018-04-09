#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <iostream>
#include <string>
#include <future>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
using namespace std;

class ThreadPool {
    public:
        ThreadPool(size_t threads): stop(false) {
            for (int i = 0; i < threads; ++i) {
                workers.emplace_back([this]{
                        for (;;) {
                            function<void()> task;
                            {
                                unique_lock<mutex> lock(this->queue_mutex);
                                this->condition.wait(lock, [this]{return this->stop || !this->tasks.empty();});
                                if (this->stop && this->tasks.empty()) return;
                                task = move(this->tasks.front());
                                this->tasks.pop();
                            }
                            task();
                        }
                        });
            }
        }

        template<typename F, typename... Args>
        auto enqueue(F&& f, Args&&... args)
        -> future<typename result_of<F(Args...)>::type>
        {
            using return_type = typename result_of<F(Args...)>::type;
            auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...));
            future<return_type> res = task->get_future();
            {
                unique_lock<mutex> lock(queue_mutex);
                if (stop) throw runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([task]{(*task)();});
            }
            condition.notify_one();
            return res;
        }

        ~ThreadPool() {
            {
                unique_lock<mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for (thread& worker: workers) {
                worker.join();
            }
        }

    private:
        vector<thread> workers;
        queue<function<void()>> tasks;
        mutex queue_mutex;
        condition_variable condition;
        bool stop;
};

#endif

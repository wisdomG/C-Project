#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <string>
#include <iostream>
#include <queue>
#include <pthread.h>
using namespace std;

/**
 * 任务父类
 * 子类重写run函数，作为线程任务
 */
class CTask {
    protected:
        string taskName;
        void* taskData;
    public:
        CTask() = default; // 使用默认构造函数
        CTask(const string& tn): taskName(tn), taskData(nullptr) {}
        virtual int run() = 0;
        void setData(void* data);
        virtual ~CTask() {}
};

/**
 * 线程池类
 */
class ThreadPool {
private:
    static queue<CTask*> taskList;
    static bool shutdown;
    int threadNum;
    pthread_t *tids;

    static pthread_mutex_t mutex;
    static pthread_cond_t cond;

protected:
    static void* threadFunc(void* data);
    //static int move2Idle(pthread_t);
    //static int move2Busy(pthread_t);
    int create();

public:
    ThreadPool(int threadNum);
    int addTask(CTask*);
    int stopAll();
    int getTaskSize();
};

#endif

#include "ThreadPool.h"
#include <cstdio>

// 静态变量类外初始化 
queue<CTask*> ThreadPool::taskList;
bool ThreadPool::shutdown = false;

pthread_mutex_t ThreadPool::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  ThreadPool::cond  = PTHREAD_COND_INITIALIZER;

void CTask::setData(void* data) {
    this->taskData = data;
}

/***********************************************************
 * 线程池构造函数
 **********************************************************/
ThreadPool::ThreadPool(int threadNum) {
    this->threadNum = threadNum;
    printf("I will create %d threads \n", threadNum);
    create();
}

/*********************************************************
 * 线程池中每个线程所干的事情就是：
 * 从任务队列中取出一个任务，进行执行
 ********************************************************/
void* ThreadPool::threadFunc(void* data) {
    pthread_t tid = pthread_self();
    while(1) {
        pthread_mutex_lock(&mutex);
        // 任务队列中没有任务，则等待条件变量
        while(taskList.size() == 0 && !shutdown)
            pthread_cond_wait(&cond, &mutex);
        // 被唤醒后，检查是否要销毁线程池
        // 是的话，则释放锁，并退出线程
        if (shutdown) {
            pthread_mutex_unlock(&mutex);
            printf("[tid: %lu]\texit\n", pthread_self());
            pthread_exit(nullptr);
        }
        printf("[tid: %lu]\trun\n ", tid);
        // 从队头中取出一个任务
        auto task = taskList.front();
        taskList.pop();
        pthread_mutex_unlock(&mutex);
        // 执行任务
        task->run();
        printf("[tid: %lu]\tidle\n", tid);
    }
}

/***********************************************************
 * 往任务队列中加入一个任务
 * 由于队列是共享资源，不管加任务还是去任务都需要先进行加锁
 **********************************************************/
int ThreadPool::addTask(CTask* task) {
    pthread_mutex_lock(&mutex);
    // 加入任务队列
    taskList.push(task);
    pthread_mutex_unlock(&mutex);
    // 放入任务后，还要释放出一个条件变量，有可能某个线程正在等待这个任务
    pthread_cond_signal(&cond);
    return 0;
}

/*************************************************************
 * 构造线程池，建立多个线程
 *************************************************************/
int ThreadPool::create() {
    // 存储线程id
    tids = new pthread_t[threadNum];
    for (int i = 0; i < threadNum; ++i) {
        // 建立多个线程，每个线程都执行threadFunc函数
        pthread_create(&tids[i], nullptr, threadFunc, nullptr);
    }
    return 0;
}

/***********************************************************
 * 销毁线程池
 * 主要是广播一个条件变量，等待其他线程技术，最后释放资源和销毁锁
 * 和条件变量
 ***********************************************************/
int ThreadPool::stopAll() {
    // 这里判断shutdown是为了防止重入
    if (shutdown) return -1;
    printf("Now we will end all threads\n\n");
    shutdown = true;
    // 唤醒所有正在等待的线程
    pthread_cond_broadcast(&cond);
    // 等待其他线程结束
    for (int i = 0; i < threadNum; ++i) {
        pthread_join(tids[i], nullptr);
    }
    delete[] tids;
    tids = nullptr;

    // 销毁互斥锁与信号量
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}

/**********************************************************
 * 获取任务队列中还有多少个任务
 * 访问时也要加锁，因为STL中的容器都不是线程安全的
 **********************************************************/
int ThreadPool::getTaskSize() {
    pthread_mutex_lock(&mutex);
    int sz = taskList.size();
    pthread_mutex_unlock(&mutex);
    return sz;
}





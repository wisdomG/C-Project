#include "ThreadPool.h"
#include <cstdio>
#include <unistd.h>
#include <stdlib.h>

/**
 * 任务子类
 * 重写父类的run函数
 */
class MyTask: public CTask {
public:
    MyTask() = default;
    int run() {
        printf("%s", taskName.c_str());
        int x = rand() % 5 + 1;
        sleep(x);
        return 0;
    }
};

int main() {
    srand(time(nullptr));

    MyTask task;
    char data[] = "hello ";
    task.setData(data);
    ThreadPool pool(5);

    for (int i = 0; i < 20; ++i)
        pool.addTask(&task);

    while (1) {
        printf("there are still %d tasks need to handle\n", pool.getTaskSize());
        if (pool.getTaskSize() == 0) {
            if (pool.stopAll() == 0) {
                printf("thread pool clear, exit \n");
                exit(0);
            }
        }
        sleep(2);
        printf("2 seconds later....\n");
    }
    return 0;
}

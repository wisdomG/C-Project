#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>

/* 基于信号量的生产者消费者模型
 */

#define LEN 10

sem_t blank, full;
int myQueue[LEN] = {0};

/* 生产者模型
 * 首先观察是否有空位置，有则进行生产并post full信号量
 * 无则阻塞等待
 */
void* producer(void* arg) {
    int p = 0;
    for(;;) {
        sem_wait(&blank);
        myQueue[p] = rand() % 100;
        printf("produce %d\n", myQueue[p]);
        sem_post(&full);
        p = (p+1) % LEN;
        sleep(rand() % 5);
    }
}

/* 消费者模型
 * 首先观察是否有数据，有则进行消费，同时post blank信号量
 * 无则阻塞等待
 */
void* consumer(void* arg) {
    int c = 0;
    for (;;) {
        sem_wait(&full);
        printf("consume %d\n", myQueue[c]);
        myQueue[c] = 0;
        sem_post(&blank);
        c = (c+1) % LEN;
        sleep(rand() % 5);
    }
}

int main(int argc, char** argv) {
    // 初始化两个信号量，一个表示空的个数，一个表示满的个数
    sem_init(&blank, 0, LEN);
    sem_init(&full, 0, 0);
    srand(time(NULL));
    pthread_t pid, cid;
    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);
    pthread_join(pid, NULL);
    pthread_join(cid, NULL);
    sem_destroy(&full);
    sem_destroy(&blank);
    return 0;
}

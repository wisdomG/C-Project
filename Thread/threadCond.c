#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* 基于条件变量的生产者消费者模型
 */

// 生产的消息以链表形式进行组织
struct msg{
    int num;
    struct msg* next;
};

struct msg* head;
int msgnum = 0;
const int max = 10; // 链表中的最大消息数目
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* 生产者模型
 * 当链表中的消息少于max时，则进行生产，生产成功后，
 * 通过条件变量通知可能正在阻塞等待的消费者
 */
void* producer(void* arg) {
    struct msg* mp;
    for(;;) {
        mp = malloc(sizeof(struct msg));
        mp->num = rand() % 100;
        printf("produce %d\n", mp->num);
        pthread_mutex_lock(&mutex);
        if (msgnum >= max){
            free(mp);
            printf("too many msg, stop producer\n");
        } else {
            mp->next = head;
            head = mp;
            ++msgnum;
        }
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
        sleep(rand() % 10);
    }
}

/* 消费者模型
 * 判断链表中是否还有消息，有则进行消费，
 * 没有则使用条件变量阻塞等待
 */
void* consumer(void* arg) {
    struct msg* mp;
    for (;;) {
        pthread_mutex_lock(&mutex);
        if (head == NULL)
        {
            printf("No msg, block and wait\n");
            pthread_cond_wait(&cond, &mutex);
            printf("get the cond, continue\n");
        }
        mp = head;
        head = mp->next;
        --msgnum;
        pthread_mutex_unlock(&mutex);
        printf("get the msg : %d\n", mp->num);
        free(mp);
        sleep(rand() % 5);
    }
}

int main(int argc, char** argv) {
    pthread_t pid, cid;
    srand(time(NULL));
    pthread_create(&pid, NULL, producer, NULL);
    pthread_create(&cid, NULL, consumer, NULL);
    pthread_join(pid, NULL);
    pthread_join(cid, NULL);
    return 0;
}

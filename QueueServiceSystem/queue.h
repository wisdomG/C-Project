#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <iostream>
#include "event.h"
using namespace std;

/* 队列模板类
 * 本实验包含两个队列：顾客队列与事件队列
 * 顾客队列直接将顾客插入到队尾
 * 事件队列则按照事件的发生顺序插入队列(优先队列？)
 */
template <class T>
class Queue {
public:
    // 该队列模板使用了头结点
    Queue() {
        this->front = new T;
        if (!this->front) {
            cout << "memory allocate failed" << endl;
            exit(-1);
        }
        this->front->next = NULL;
        this->back = this->front;
    }

    // 先清空队列，再删除头结点
    ~Queue() {
        this->clearQueue();
        delete this->front;
    }

    // 清空队列
    void clearQueue() {
        T *tmp = this->front->next;
        while (tmp) {
            T *tmpdel = tmp;
            tmp = tmp->next;
            delete tmpdel;
        }
        this->front->next = NULL;
        this->back = this->front;
    }

    // 入队，插入队尾
    T* enqueue(T &node) {
        T *new_node = new T;
        if (!new_node) {
            cout << "memory allocate failed" << endl;
            exit(-1);
        }
        *new_node = node;
        this->back->next = new_node;
        this->back = new_node;
        return this->front;
    }

    // 出队，如果出队的是尾结点，则要修改back指针的指向
    T* dequeue() {
        if (this->front->next == NULL) return NULL;
        T *ret = this->front->next;
        this->front->next = ret->next;
        if (ret == this->back) {
            this->back = this->front;
        }
        return ret;
    }

    // 将一个事件入队，按照事件发生的先后顺序插入队中
    T* orderEnqueue(Event &event) {
        Event *e = new Event;
        if (!e) {
            cout << "memory allocate failed" << endl;
            exit(-1);
        }
        *e = event;
        if (this->front->next == NULL) {
            this->enqueue(event);
            return this->front;
        }
        Event *tmp_list = this->front;
        while (tmp_list->next && tmp_list->next->occur_time < e->occur_time) {
            tmp_list = tmp_list->next;
        }
        e->next = tmp_list->next;
        tmp_list->next = e;
        return this->front;
    }

    // 队中元素个数
    int length() {
        T* tmp = this->front;
        int len = 0;
        while (tmp) {
            ++len;
            tmp = tmp->next;
        }
        return len - 1;
    }

private:
    // 队首与队尾指针
    T *front;
    T *back;

};

#endif

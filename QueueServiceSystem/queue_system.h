#ifndef _QUEUE_SYSTEM_H
#define _QUEUE_SYSTEM_H

#include "queue.h"
#include "event.h"
#include "service_window.h"
#include "customer.h"

#define INTERVAL 30

class QueueSystem {
private:
    int window_num;
    int total_service_time;
    int total_customer_stay_time;
    int total_customer_num;

    ServiceWindow *windows;
    Queue<Customer> customer_list;
    Queue<Event> event_list;
    Event *current_event;

    double avg_customers;
    double avg_stay_time;

    // 初始化
    void init() {
        // 新建一个事件，默认为用户到达事件，同时随机生成了事件发生时间
        struct Event *event = new Event;
        current_event = event;
    }

    // 返回平均等待时间
    double run() {
        this->init();
        // 观察事件列表中是否还有事件，有则根据事件类型做出不同的响应
        while (current_event) {
            if (current_event->event_type == -1) {
                customerArrived();
            } else {
                customerDeparture();
            }
            delete current_event;
            // 事件处理完之后就出队
            current_event = event_list.dequeue();
        }
        this->end();
        return (double)(total_customer_stay_time / total_customer_num);
    }

    // 结束后的清理工作：清空队列，设置窗口空闲
    void end() {
        for (int i = 0; i < window_num; ++i) {
            windows[i].setIdle();
        }
        customer_list.clearQueue();
        event_list.clearQueue();
    }

    // 遍历每个窗口的当前状态，获取空闲窗口索引
    int getIdelServiceWindow() {
        for (int i = 0; i < window_num; ++i) {
            if (windows[i].isIdle()) return i;
        }
        return -1;
    }

    // 处理顾客到达事件
    void customerArrived() {
        // 顾客数递增
        ++total_customer_num;
        // 随机一个时间间隔，模拟下次用户到达事件
        int intertime = Random::uniform(INTERVAL);
        int time = current_event->occur_time + intertime;
        struct Event tmp_event(time);
        // 如果新生成的用户到达事件的用户达到时间在服务时间内，则入队准备接受服务
        if (time < total_service_time) {
            event_list.orderEnqueue(tmp_event);
        }
        // 利用当前事件的到达事件实例化一个顾客，同时随机生成了该用户所需的服务时间
        Customer *customer = new Customer(current_event->occur_time);
        if (!customer) {
            cout << "memory allocate failed" << endl;
            exit(-1);
        }
        // 将当前事件所对应的顾客入队
        customer_list.enqueue(*customer);

        // 找到空闲窗口并分配给该顾客
        int idleIndex = getIdelServiceWindow();
        if (idleIndex >= 0) {
            // 顾客出队
            customer = customer_list.dequeue();
            // 设置窗口状态
            windows[idleIndex].serveCustomer(*customer);
            windows[idleIndex].setBusy();
            // 向事件队列中插入顾客离开事件
            struct Event tmp_event(current_event->occur_time + customer->duration, idleIndex);
            event_list.orderEnqueue(tmp_event);
        } else {
            cout << "No idle window" << endl;
        }
        delete customer;
    }

    // 处理顾客离开事件
    void customerDeparture() {
        // 客户离开时，银行还没下班
        if (current_event->occur_time < total_service_time) {
            // 客户停留时间=离开时间-客户到达时间
            total_customer_stay_time += current_event->occur_time - windows[current_event->event_type].getCustomerArriveTime();
            // 如果有待服务的客户，则继续服务，否则设置窗口空闲
            if (customer_list.length()) {
                // 弹出客户并服务，同时将离开事件加入队列
                Customer *customer = customer_list.dequeue();
                windows[current_event->event_type].serveCustomer(*customer);
                Event tmp_event(current_event->occur_time + customer->duration, current_event->event_type);
                event_list.orderEnqueue(tmp_event);
                delete customer;
            } else {
                windows[current_event->event_type].setIdle();
            }
        } else {
            // 已经过了下班时间，不再进行服务，只需将事件队列中的时间处理完即可
        }
    }
public:
    QueueSystem(int total_service_time, int window_num) :
        total_service_time(total_service_time), window_num(window_num),
        total_customer_stay_time(0), total_customer_num(0) {
            this->windows = new ServiceWindow[window_num];
        }

    ~QueueSystem() {
        delete[] windows;
    }

    void simulate(int simulate_num) {
        double sum = 0;
        for (int i = 0; i < simulate_num; ++i) {
            sum += run();
        }
        avg_stay_time = (double)(sum) / simulate_num;
        avg_customers = (double)(total_customer_num) / (total_service_time * simulate_num);
    }

    inline double getAvgStayTime() const {
        return avg_stay_time;
    }

    inline double getAvgCustomer() const {
        return avg_customers;
    }
 };

#endif

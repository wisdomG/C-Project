#ifndef _CUSTOMER_H_
#define _CUSTOMER_H_

#include "random.h"

#define SERVICE_TIME_MAX 30

struct Node {
    int duration;
    int arriveTime;
    struct Node *next;

    // 顾客到达事件和服务时长
    Node (int arriveTime = 0, int duration = Random::uniform(SERVICE_TIME_MAX)) :
        arriveTime(arriveTime), duration(duration), next(NULL) {}
};

typedef struct Node Node;
typedef struct Node Customer;

#endif

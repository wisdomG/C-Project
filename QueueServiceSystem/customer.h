#ifndef _CUSTOMER_H_
#define _CUSTOMER_H_

#include "random.h"

#define SERVICE_TIME_MAX 30

struct Node {
    int duration;
    int arriveTime;
    struct Node *next;

    Node (int arriveTime = 0, int duration = Random::uniform(SERVICE_TIME_MAX)) :
        arriveTime(arriveTime), duration(duration), next(NULL) {}
};

typedef struct Node Node;
typedef struct Node Customer;

#endif

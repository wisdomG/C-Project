#ifndef _EVENT_H_
#define _EVENT_H_

#include "random.h"

#define RANDOM_PARAMETER 30

/* 事件类
 * 事件驱动的设计方法
 * 包含两种事件：顾客到达事件和顾客离开事件
 */
struct Event {
    // 事件的发生事件
    int occur_time;
    // 事件类型，-1表示达到事件，>=0表示离开事件，且其值表示离开的窗口号
    int event_type;
    struct Event *next;
    // 默认参数
    Event(int occur_time = Random::uniform(RANDOM_PARAMETER), int event_type = -1) :
        occur_time(occur_time), event_type(event_type), next(NULL) {}
};

#endif

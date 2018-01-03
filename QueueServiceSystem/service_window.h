#ifndef _SERVICE_WINDOW_H_
#define _SERVICE_WINDOW_H_

#include "customer.h"

enum WindomStatus {
    SERVICE,
    IDLE,
};

class ServiceWindow {
private:
    // 窗口状态与该窗口服务的顾客
    WindomStatus window_status;
    Customer customer;

public:
    inline ServiceWindow() {
        window_status = IDLE;
    }

    inline bool isIdle() const {
        return window_status == IDLE;
    }

    inline void serveCustomer(Customer &customer) {
        this->customer = customer;
    }

    inline void setBusy() {
        window_status = SERVICE;
    }

    inline void setIdle() {
        window_status = IDLE;
    }

    inline int getCustomerArriveTime() const {
        return customer.arriveTime;
    }

    inline int getCustomerDuration() const {
        return customer.duration;
    }
};

#endif




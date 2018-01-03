#include <iostream>
#include <ctime>
#include "queue_system.h"

int main() {
    srand(time(NULL));
    int total_service_time = 480;
    int window_num = 1;
    int simulate_num = 100000;

    QueueSystem system(total_service_time, window_num);
    system.simulate(simulate_num);

    cout << "The average time of customer stay in bank: "
        << system.getAvgStayTime() << endl;

    cout << "The number of customers arrive bank per minutes: "
        << system.getAvgCustomer() << endl;

    return 0;
}


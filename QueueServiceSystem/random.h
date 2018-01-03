#ifndef _RANDOM_H_
#define _RANDOM_H_

#include <iostream>

class Random{
public:
    static double uniform(double max = 1) {
        return ((double)(rand()) / RAND_MAX) * max;
    }
};

#endif

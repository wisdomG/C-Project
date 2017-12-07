//
// Created by wise on 2017/12/6.
//

#ifndef LEAKDETECTOR_LEAKDETECTOR_H
#define LEAKDETECTOR_LEAKDETECTOR_H

#include <iostream>

void* operator new(size_t _size);
void* operator new[](size_t _size);
void* operator new(size_t _size, const char *_file, unsigned int _line);
void* operator new[](size_t _size, const char *_file, unsigned int _line);
void operator delete(void* ptr);
void operator delete[](void* ptr);

class _leak_detector {
public:
    static unsigned int callCnt;
    _leak_detector() noexcept {
        ++callCnt;
        std::cout << "callcnt " << callCnt << std::endl;
    }
    ~_leak_detector() noexcept {
        std::cout << "~callcnt " << callCnt << std::endl;
        if (--callCnt == 0)
            LeakDetector();
    }

private:
    static unsigned int LeakDetector() noexcept ;
};

static _leak_detector _exit_counter;

#endif //LEAKDETECTOR_LEAKDETECTOR_H

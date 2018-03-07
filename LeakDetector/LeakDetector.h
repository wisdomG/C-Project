//
// Created by wise on 2017/12/6.
//

#ifndef LEAKDETECTOR_LEAKDETECTOR_H
#define LEAKDETECTOR_LEAKDETECTOR_H

#include <iostream>

// 重载new和delete运算符
void* operator new(size_t _size);
void* operator new[](size_t _size);
void* operator new(size_t _size, const char *_file, unsigned int _line);
void* operator new[](size_t _size, const char *_file, unsigned int _line);
void operator delete(void* ptr);
void operator delete[](void* ptr);

class _leak_detector {
private:
    unsigned int LeakDetector(void) noexcept ;

public:
    static unsigned int callCnt;
    _leak_detector() noexcept {
        ++callCnt;
        std::cout << "callcnt " << callCnt << std::endl;
    }
    ~_leak_detector() noexcept {
        std::cout << "~callcnt " << callCnt << std::endl;
        // LeakDetector函数只调用一次
        if (--callCnt == 0)
            LeakDetector();
    }
};

// 声明一个静态对象，程序结束时会执行该对象的析构函数，从而完成内存检查器的目标
static _leak_detector _exit_counter;

#endif //LEAKDETECTOR_LEAKDETECTOR_H

#include <iostream>
#include <ctime>
#include <cassert>
#include "StackAlloc.h"
#include "MemoryPool.h"
using namespace std;

#define ELEMS 100
#define REPS 50

class Test{
    public:
        Test() {}
        ~Test() {}
    private:
        int testInt;
        double testDouble;
};


int main() {
    clock_t start, end;
    // 使用默认的内存分配器，参考STL源码
    StackAlloc<int, allocator<int>> stackDef;
    start = clock();
    for (int i = 0; i < REPS; ++i) {
        for (int j = 0; j < ELEMS; ++j)
            stackDef.push(j);
        for (int j = 0; j < ELEMS; ++j)
            stackDef.pop();
    }
    end = clock();
    cout << "Default Alloctor Time: ";
    cout << (end - start) << "ms" << endl;

    // 使用我们编写的内存池进行内存分配
    StackAlloc<int, MemoryPool<int>> stackMP;
    start = clock();
    for (int i = 0; i < REPS; ++i) {
        for (int j = 0; j < ELEMS; ++j)
            stackMP.push(j);
        for (int j = 0; j < ELEMS; ++j)
            stackMP.pop();
    }
    end = clock();
    cout << "MemoryPool Alloctor Time: ";
    cout << (end - start) << "ms" << endl;

    // 在测试一下对类进行内存分配
    StackAlloc<Test, MemoryPool<Test>> stackMPTest;
    start = clock();
    for (int i = 0; i < REPS; ++i) {
        for (int j = 0; j < ELEMS; ++j)
            stackMPTest.push(Test());
        for (int j = 0; j < ELEMS; ++j)
            stackMPTest.pop();
    }
    end = clock();
    cout << "MemoryPool Alloctor Time: ";
    cout << (end - start) << "ms" << endl;
    return 0;
}

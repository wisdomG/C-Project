#include <iostream>
#include <ctime>
#include <cassert>
#include "StackAlloc.h"
#include "MemoryPool.h"
using namespace std;

#define ELEMS 1e6
#define REPS 50

int main() {
    clock_t start, end;
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

    StackAlloc<int, MemoryPool<int>> stackMP;
    start = clock();
//    for (int i = 0; i < REPS; ++i) {
//        for (int j = 0; j < ELEMS; ++j)
//            stackMP.push(j);
//        for (int j = 0; j < ELEMS; ++j)
//            stackMP.pop();
//    }
    stackMP.push(100);
    stackMP.push(10);
    end = clock();
    cout << "MemoryPool Alloctor Time: ";
    cout << (end - start) << "ms" << endl;
    return 0;
}
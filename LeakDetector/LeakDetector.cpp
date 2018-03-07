//
// Created by wise on 2017/12/6.
//
#include "LeakDetector.h"
#include <iostream>
#include <cstring>

// 源代码使用的双向链表，主要是在删除的时候比较方便
typedef struct _MemoryList{
    struct _MemoryList *next, *prev;
    size_t size;
    bool isArray;
    char *file;
    unsigned int line;
} _MemoryList;

static unsigned long _memory_allocated = 0;
unsigned int _leak_detector::callCnt = 0;

// 虚拟头结点
static _MemoryList _root = {
        //&_root, &_root,
        nullptr, nullptr,
        0, false, nullptr, 0
};

// 分配内存
void* AllocateMemory(size_t _size, bool _array, const char *_file, unsigned int _line) {

    // 申请两部分空间，前面是节点，后面是实际给用户的空间
    size_t newSize = sizeof(_MemoryList) + _size;
    _MemoryList *newEle = reinterpret_cast<_MemoryList*>(malloc(newSize));

    // 将新申请的内存加入到根节点之后，即新申请的节点总是位置链表头部的位置，即除了虚拟头结点的第一个节点
    newEle->next = _root.next;
    newEle->prev = &_root;
    //_root.next->prev = newEle;
    _root.next = newEle;

    // 初始化节点内部变量
    newEle->size = _size;
    newEle->isArray = _array;
    newEle->line = _line;
    newEle->file = nullptr;

    if (_file != nullptr) {
        newEle->file = reinterpret_cast<char *>(malloc(strlen(_file)+1));
        strcpy(newEle->file, _file);
    }

    // 记录已申请的内存空间
    _memory_allocated += _size;

    // 返回给用户使用的空间
    return reinterpret_cast<char *>(newEle) + sizeof(_MemoryList);
}

void DeleteMemory(void* _ptr, bool _array) {

    // 找到对应的节点指针，并更新链表结构
    _MemoryList *curr = reinterpret_cast<_MemoryList*>(reinterpret_cast<char *>(_ptr) - sizeof(_MemoryList));
    if (curr->isArray != _array) return ;
    curr->prev->next = curr->next;
    curr->next->prev = curr->prev;

    // 更新已分配的内存量
    _memory_allocated -= curr->size;

    // 释放内存
    if (curr->file != nullptr) free(curr->file);
    free(curr);
}

// 重载new和delete运算符，内部使用我们的内存分配函数实现
void* operator new(size_t _size) {
    std::cout << "new1 " << _size << std::endl;
    return AllocateMemory(_size, false, nullptr, 0);
}

void* operator new[](size_t _size) {
    std::cout << "new1[] " << _size << std::endl;
    return AllocateMemory(_size, true, nullptr, 0);
}

void* operator new(size_t _size, const char *_file, unsigned int _line) {
    std::cout << "new2 " << _size << std::endl;
    return AllocateMemory(_size, false, _file, _line);
}

void* operator new[](size_t _size, const char *_file, unsigned int _line) {
    std::cout << "new2[] " << _size << std::endl;
    return AllocateMemory(_size, true, _file, _line);
}

void operator delete(void *_ptr) noexcept {
    std::cout << "delete " << std::endl;
    DeleteMemory(_ptr, false);
}

void operator delete[](void *_ptr) noexcept {
    std::cout << "delete[] " << std::endl;
    DeleteMemory(_ptr, true);
}

unsigned int _leak_detector::LeakDetector(void) noexcept {
    unsigned int cnt = 0;
    _MemoryList *ptr = _root.next;
    while (ptr != nullptr) {
        if (ptr->isArray) std::cout << "leak[] ";
        else std::cout << "leak ";
        std::cout << ptr << " size " << ptr->size ;
        if (ptr->file) std::cout << " (locate in " << ptr->file << " line " << ptr->line << ")" << std::endl;
        else std::cout << " can not find the position" << std::endl;

        ++cnt;
        ptr = ptr->next;
    }
    if (cnt) std::cout << "Total " << cnt << " leaks, size " << _memory_allocated << " byte. " << std::endl;
    return cnt;
}

//
// Created by wise on 2017/12/1.
//

#ifndef MEMORYPOOL_STACKALLOC_H
#define MEMORYPOOL_STACKALLOC_H

#include <memory>
template <typename T>
struct StackNode_ {
    T data;
    StackNode_ *prev;
};

template <typename T, typename Alloc = std::allocator<T>>
class StackAlloc {
public:
    typedef StackNode_<T> Node;
    typedef typename Alloc::template rebind<Node>::other allocator;

    StackAlloc() {head_ = nullptr; };
    ~StackAlloc() {clear(); };

    bool empty() {return (head_ == nullptr); };
    void clear();
    void push(T ele);
    T pop();
    T top() { return head_->data; };

private:
    allocator allocator_;
    Node* head_;
};

template <typename T, typename Alloc>
void StackAlloc<T, Alloc>::clear() {
    Node* cur = head_;
    while (cur != nullptr) {
        Node *tmp = cur->prev;
        allocator_.destroy(cur);
        allocator_.deallocate(cur, 1);
        cur = tmp;
    }
}

template <typename T, typename Alloc>
void StackAlloc<T, Alloc>::push(T ele) {
    Node* newNode = allocator_.allocate(1);
    allocator_.construct(newNode, Node());
    newNode->data = ele;
    newNode->prev = head_;
    head_ = newNode;
}

template <typename T, typename Alloc>
T StackAlloc<T, Alloc>::pop() {
    T res = head_->data;
    Node* tmp = head_->prev;
    allocator_.destroy(head_);
    allocator_.deallocate(head_, 1);
    head_ = tmp;
    return res;
};

/* std:allocator 的分配和销毁过程分为4步
 * 1 使用allocate分配空间，返回空间指针
 * 2 使用construct在指针位置构造一个对象
 * 3 使用destroy析构对象
 * 4 使用deallocate释放申请的空间
 * */

#endif //MEMORYPOOL_STACKALLOC_H

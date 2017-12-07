//
// Created by wise on 2017/12/4.
//

#ifndef MEMORYPOOL_MEMORYPOOL_H
#define MEMORYPOOL_MEMORYPOOL_H

#include <climits>
#include <cstddef>
#include <utility>
#include <new>
#include <stdint-gcc.h>

template <typename T, size_t BlockSize=4096>
class MemoryPool {
public:
    typedef T* pointer;
    template <typename U> struct rebind {
        typedef MemoryPool<U> other;
    };
    MemoryPool() noexcept {
        currentBlock_ = nullptr;
        currentSlot_ = nullptr;
        lastSlot_ = nullptr;
        freeSlots_ = nullptr;
    };
    ~MemoryPool() noexcept ;
    pointer allocate(size_t n = 1, const T* hint = 0);
    void deallocate(pointer p, size_t n = 1);

    template <typename U>
    void destroy(U* p) { p->~U(); }

    template <typename U, typename... Args>
    void construct(U *p, Args&&... args);

private:
    union Slot_ {
        Slot_* next;
        T element;
    };
    typedef char*  data_pointer_;
    typedef Slot_  slot_type_;
    typedef Slot_* slot_pointer_;

    slot_pointer_ currentBlock_;
    slot_pointer_ currentSlot_;
    slot_pointer_ lastSlot_;
    slot_pointer_ freeSlots_; // 用来收集释放的内存，进行再利用

    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small");
};

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept {
    slot_pointer_ curr = currentBlock_;
    while (curr != nullptr) {
        slot_pointer_ tmp = curr->next;
        operator delete(reinterpret_cast<slot_pointer_ >(curr));
        curr = tmp;
    }
}

template <typename T, size_t BlockSize>
T* MemoryPool<T, BlockSize>::allocate(size_t n, const T *hint) {
    if (freeSlots_ != nullptr) {
        pointer result = reinterpret_cast<pointer >(freeSlots_);
        freeSlots_ = freeSlots_->next;
        std::cout << "----freeSlot_" << std::endl;
        return result;
    } else {
        if (currentSlot_ >= lastSlot_) {
            data_pointer_  newBlock = reinterpret_cast<data_pointer_ >(operator new(BlockSize));
            reinterpret_cast<slot_pointer_ >(newBlock)->next = currentBlock_;
            currentBlock_ = reinterpret_cast<slot_pointer_ >(newBlock);

            data_pointer_ body = newBlock + sizeof(slot_pointer_);
            uintptr_t result = reinterpret_cast<uintptr_t >(body);
            // 满足对齐要求
            size_t bodyPadding = (alignof(slot_type_) - result) % alignof(slot_type_);
            currentSlot_ = reinterpret_cast<slot_pointer_ >(body + bodyPadding);
            lastSlot_ = reinterpret_cast<slot_pointer_ >(newBlock + BlockSize - sizeof(slot_type_));
            //freeSlots_ = currentSlot_->next;

        }
        return reinterpret_cast<pointer >(currentSlot_++);
    }
}

template <typename T, size_t BlockSize>
template <typename U, typename... Args>
void MemoryPool<T, BlockSize>::construct(U *p, Args&&... args) {
    new (p) U(std::forward<Args>(args)...);
}

template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::deallocate(pointer p, size_t n) {
    if (p != nullptr) {
        reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
        freeSlots_ = reinterpret_cast<slot_pointer_>(p);
    }
}





#endif //MEMORYPOOL_MEMORYPOOL_H

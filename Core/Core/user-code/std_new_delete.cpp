/*
 * litesys_std_new_delete.cpp
 *
 *  Created on: Dec 19, 2024
 *      Author: DrL潇湘
 */

#include "cpp_public.hpp"
#include <cstdio>

void *operator new(std::size_t count) {
    if (count == 0)
        count++;
    void *ptr = pvPortMalloc(count);
#ifdef _GLIBCXX_ENABLE_EXCEPTIONS
    if (ptr == nullptr)
        throw std::bad_alloc {};
#else
    if (ptr == nullptr)
        return ptr;
#endif
    return ptr;
}
void *operator new[](std::size_t count) {
    if (count == 0)
        count++;
    void *ptr = pvPortMalloc(count);
#ifdef _GLIBCXX_ENABLE_EXCEPTIONS
    if (ptr == nullptr)
        throw std::bad_alloc {};
#else
    if (ptr == nullptr)
        return ptr;
#endif
    return ptr;
}
void *operator new(std::size_t count, std::align_val_t al) {
    void *ptr = pvPortMalloc(count + ((size_t)al) + sizeof(void *));
#ifdef _GLIBCXX_ENABLE_EXCEPTIONS
    if (ptr == nullptr)
        throw std::bad_alloc {};
#else
    if (ptr == nullptr)
        return ptr;
#endif

    void *aligned_ptr                                     = (void *)(((uintptr_t)ptr + ((size_t)al) + sizeof(void *)) & ~(((size_t)al) - 1));
    *((void **)((uintptr_t)aligned_ptr - sizeof(void *))) = ptr;

    return aligned_ptr;
}
void *operator new[](std::size_t count, std::align_val_t al) {
    void *ptr = pvPortMalloc(count + ((size_t)al) + sizeof(void *));
#ifdef _GLIBCXX_ENABLE_EXCEPTIONS
    if (ptr == nullptr)
        throw std::bad_alloc {};
#else
    if (ptr == nullptr)
        return ptr;
#endif

    void *aligned_ptr                                     = (void *)(((uintptr_t)ptr + ((size_t)al) + sizeof(void *)) & ~(((size_t)al) - 1));
    *((void **)((uintptr_t)aligned_ptr - sizeof(void *))) = ptr;

    return aligned_ptr;
}
void *operator new(std::size_t count, const std::nothrow_t &tag) noexcept {
    if (count == 0)
        count++;
    void *ptr = pvPortMalloc(count);
    return ptr;
}
void *operator new[](std::size_t count, const std::nothrow_t &tag) noexcept {
    if (count == 0)
        count++;
    void *ptr = pvPortMalloc(count);
    return ptr;
}
void *operator new(std::size_t count, std::align_val_t al, const std::nothrow_t &tag) noexcept {
    void *ptr = pvPortMalloc(count + ((size_t)al) + sizeof(void *));
    if (ptr == nullptr)
        return ptr;

    void *aligned_ptr                                     = (void *)(((uintptr_t)ptr + ((size_t)al) + sizeof(void *)) & ~(((size_t)al) - 1));
    *((void **)((uintptr_t)aligned_ptr - sizeof(void *))) = ptr;

    return aligned_ptr;
}
void *operator new[](std::size_t count, std::align_val_t al, const std::nothrow_t &tag) noexcept {
    void *ptr = pvPortMalloc(count + ((size_t)al) + sizeof(void *));
    if (ptr == nullptr)
        return ptr;

    void *aligned_ptr                                     = (void *)(((uintptr_t)ptr + ((size_t)al) + sizeof(void *)) & ~(((size_t)al) - 1));
    *((void **)((uintptr_t)aligned_ptr - sizeof(void *))) = ptr;

    return aligned_ptr;
}

void operator delete(void *ptr) noexcept {
    vPortFree(ptr);
}
void operator delete[](void *ptr) noexcept {
    vPortFree(ptr);
}
void operator delete(void *ptr, std::align_val_t al) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}
void operator delete[](void *ptr, std::align_val_t al) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}
void operator delete(void *ptr, std::size_t sz) noexcept {
    vPortFree(ptr);
}
void operator delete[](void *ptr, std::size_t sz) noexcept {
    vPortFree(ptr);
}
void operator delete(void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}
void operator delete[](void *ptr, std::size_t sz, std::align_val_t al) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}
void operator delete(void *ptr, const std::nothrow_t &tag) noexcept {
    vPortFree(ptr);
}
void operator delete[](void *ptr, const std::nothrow_t &tag) noexcept {
    vPortFree(ptr);
}
void operator delete(void *ptr, std::align_val_t al, const std::nothrow_t &tag) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}
void operator delete[](void *ptr, std::align_val_t al, const std::nothrow_t &tag) noexcept {
    void *unaligned_ptr = *((void **)((uintptr_t)ptr - sizeof(void *)));
    vPortFree(unaligned_ptr);
}

/*
 * memory_pool.c
 *
 *  Created on: Dec 15, 2024
 *      Author: DrL潇湘
 */

#include "memory_pool.hpp"

#undef ALIGN
#undef PTR_INC
#undef PTR_DEC
#undef PTR_DIFF
#undef MODULE_SIZE

#define ALIGN(ptr, length)   ((((uintptr_t)(ptr)) + ((uintptr_t)(((uintptr_t)(length)) - 1))) & (~((uintptr_t)(((uintptr_t)(length)) - 1))))
#define PTR_INC(ptr, length) ((void *)(((uintptr_t)(ptr)) + ((uintptr_t)(length))))
#define PTR_DEC(ptr, length) ((void *)(((uintptr_t)(ptr)) - ((uintptr_t)(length))))
#define PTR_DIFF(ptr1, ptr2) (((uintptr_t)(ptr1)) - ((uintptr_t)(ptr2)))
#define MODULE_SIZE          ((uintptr_t)sizeof(memory_pool_context_block))

void memory_pool_init_context(memory_pool_context *mpc, void *memory, uintptr_t length) {
    assert(mpc != nullptr);
    assert(memory != nullptr);
    assert(length != 0);
    mpc->memory          = memory;
    mpc->len             = length;
    mpc->state           = 0;
    mpc->first           = nullptr;

    void *aligned_memory = (void *)ALIGN(memory, 4);
    auto *mpb            = new (aligned_memory) memory_pool_context_block {};

    mpb->next            = nullptr;
    mpb->prev            = nullptr;
    mpb->state           = 0;

    mpc->first           = mpb;
    mpc->state           = 1;
    mpc->mpc_mutex       = xSemaphoreCreateMutex();
    ADD_SEMAQUEUE(mpc->mpc_mutex, Memory_pool_mutex);
}
void memory_pool_deinit_context(memory_pool_context *mpc) {
    assert(mpc != nullptr);
    if (in_handler_mode()) {
        return;
    }
    else {
        xSemaphoreTake(mpc->mpc_mutex, portMAX_DELAY);
    }
    mpc->memory = nullptr;
    mpc->len    = 0;
    mpc->state  = 0;
    mpc->first  = nullptr;
    DEL_SEMAQUEUE(mpc->mpc_mutex);
    vSemaphoreDelete(mpc->mpc_mutex);
    mpc->mpc_mutex = nullptr;
}
void *memory_pool_malloc_context(memory_pool_context *mpc, uintptr_t length) {
    assert(mpc != nullptr);

    if (mpc->state == 0 || length == 0 || mpc->memory == nullptr)
        return NULL;

    if (in_handler_mode())
        return NULL;
    xSemaphoreTake(mpc->mpc_mutex, portMAX_DELAY);

    void                      *res           = nullptr;
    memory_pool_context_block *curr_mpb      = mpc->first;

    uintptr_t                  needed_length = 0;
    uintptr_t                  curr_len      = 0;

    while (curr_mpb != nullptr) {
        if (curr_mpb->next) {
            curr_len = PTR_DIFF(curr_mpb->next, curr_mpb) - MODULE_SIZE;
        }
        else {
            curr_len = mpc->len - PTR_DIFF(curr_mpb, mpc->memory) - MODULE_SIZE;
        }
        if ((curr_len >= length) && (curr_mpb->state == 0)) { // length is longer than required and the state is idle
            needed_length = ALIGN(length, 4) + MODULE_SIZE;

            if ((needed_length + 4) <= curr_len) {
                auto *new_mpb  = new (PTR_INC(curr_mpb, needed_length)) memory_pool_context_block {};
                new_mpb->state = 0;
                new_mpb->prev  = curr_mpb;
                new_mpb->next  = curr_mpb->next;
                if (new_mpb->next)
                    new_mpb->next->prev = new_mpb;
                curr_mpb->next = new_mpb;
            }

            curr_mpb->state = 1;
            res             = PTR_INC(curr_mpb, MODULE_SIZE);
            break;
        }
        curr_mpb = curr_mpb->next;
    }
    xSemaphoreGive(mpc->mpc_mutex);
    return res;
}
void memory_pool_free_context(memory_pool_context *mpc, void *ptr) {
    assert(mpc != NULL);
    if (!ptr || mpc->memory == nullptr || mpc->state == 0)
        return;
    if ((uintptr_t)ptr < (uintptr_t)mpc->memory)
        return;
    if (PTR_DIFF(ptr, mpc->memory) >= mpc->len)
        return;

    if (in_handler_mode()) {
        return;
    }
    else {
        xSemaphoreTake(mpc->mpc_mutex, portMAX_DELAY);
    }
    auto *curr_mpb  = (memory_pool_context_block *)PTR_DEC(ptr, MODULE_SIZE);
    curr_mpb->state = 0;
    if (curr_mpb->next && ((curr_mpb->next->state) == 0)) {
        if (curr_mpb->next->next)
            curr_mpb->next->next->prev = curr_mpb;
        curr_mpb->next = curr_mpb->next->next;
    }

    if (curr_mpb->prev && ((curr_mpb->prev->state) == 0)) {
        if (curr_mpb->next)
            curr_mpb->next->prev = curr_mpb->prev;
        curr_mpb->prev->next = curr_mpb->next;
    }
    xSemaphoreGive(mpc->mpc_mutex);
}
uintptr_t memory_pool_remain_context(memory_pool_context *mpc) {
    assert(mpc != nullptr);
    if (mpc->memory == nullptr || mpc->state == 0)
        return 0;

    if (in_handler_mode()) {
        return 0;
    }
    else {
        xSemaphoreTake(mpc->mpc_mutex, portMAX_DELAY);
    }
    uintptr_t                  res      = 0;
    memory_pool_context_block *curr_mpb = mpc->first;
    uintptr_t                  curr_len = 0;
    while (curr_mpb != nullptr) {
        curr_len = (curr_mpb->next) ? (PTR_DIFF(curr_mpb->next, curr_mpb) - MODULE_SIZE) : (mpc->len - PTR_DIFF(curr_mpb, mpc->memory) - MODULE_SIZE);
        res += curr_len;
        curr_mpb = curr_mpb->next;
    }
    xSemaphoreGive(mpc->mpc_mutex);
    return res;
}
void memory_pool_clear_context(memory_pool_context *mpc) {
    assert(mpc != nullptr);
    if (mpc->memory == nullptr || mpc->state == 0)
        return;
    if (in_handler_mode()) {
        return;
    }
    else {
        xSemaphoreTake(mpc->mpc_mutex, portMAX_DELAY);
    }
    void *aligned_memory = (void *)ALIGN(mpc->memory, 4);
    auto *mpb            = new (aligned_memory) memory_pool_context_block {};
    mpb->next            = nullptr;
    mpb->prev            = nullptr;
    mpb->state           = 0;

    mpc->first           = mpb;
    mpc->state           = 1;
    xSemaphoreGive(mpc->mpc_mutex);
}

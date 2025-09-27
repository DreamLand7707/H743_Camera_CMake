/*
 * memory_pool.h
 *
 *  Created on: Dec 15, 2024
 *      Author: DrL潇湘
 */

#ifndef MEMORY_POOL_MEMORY_POOL_H_
#define MEMORY_POOL_MEMORY_POOL_H_

#include "prj_header.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct memory_pool_context_block_ memory_pool_context_block;
    typedef struct memory_pool_context_       memory_pool_context;

    struct memory_pool_context_block_ {
        memory_pool_context_block *prev;
        memory_pool_context_block *next;
        uintptr_t                  state;
    };

    struct memory_pool_context_ {
        void                      *memory;
        uintptr_t                  len;
        memory_pool_context_block *first;
        uintptr_t                  state;
        SemaphoreHandle_t          mpc_mutex;
    };


    void      memory_pool_init_context(memory_pool_context *mpc, void *memory, uintptr_t length);
    void      memory_pool_deinit_context(memory_pool_context *mpc);
    void     *memory_pool_malloc_context(memory_pool_context *mpc, uintptr_t length);
    void      memory_pool_free_context(memory_pool_context *mpc, void *ptr);
    uintptr_t memory_pool_remain_context(memory_pool_context *mpc);
    void      memory_pool_clear_context(memory_pool_context *mpc);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_POOL_MEMORY_POOL_H_ */

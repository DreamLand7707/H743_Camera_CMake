/*
 * memory_pool_block.hpp
 *
 *  Created on: Feb 20, 2025
 *      Author: DrL潇湘
 */

#ifndef MEMORY_POOL_BLOCK_MEMORY_POOL_BLOCK_HPP_
#define MEMORY_POOL_BLOCK_MEMORY_POOL_BLOCK_HPP_

#include "prj_header.hpp"

typedef struct memory_pool_block_lite_t memory_pool_block_lite;
typedef struct memory_pool_block_t      memory_pool_block;

#define LITESYS_MPB                     ((uintptr_t)2)
#define LITESYS_MPB_LITE                ((uintptr_t)0)
#define LITESYS_ALLOC                   ((uintptr_t)1)

#define ALIGN(ptr, length)              ((((uintptr_t)(ptr)) + ((uintptr_t)(((uintptr_t)(length)) - 1))) & (~((uintptr_t)(((uintptr_t)(length)) - 1))))
#define PTR_INC(ptr, length)            ((void *)(((uintptr_t)(ptr)) + ((uintptr_t)(length))))
#define PTR_DEC(ptr, length)            ((void *)(((uintptr_t)(ptr)) - ((uintptr_t)(length))))
#define PTR_DIFF(ptr1, ptr2)            ((((uintptr_t)(ptr1)) > ((uintptr_t)(ptr2))) ? ((uintptr_t)(ptr1)) - ((uintptr_t)(ptr2)) : ((uintptr_t)(ptr2)) - ((uintptr_t)(ptr1)))
#define MODULE_SIZE                     ((uintptr_t)sizeof(memory_pool_block))
#define LITE_MODULE_SIZE                ((uintptr_t)sizeof(memory_pool_block_lite))

#define IS_LITE_LAST(ptr)               (((ptr)->next == nullptr) || ((ptr)->next->state & LITESYS_MPB))

// level
#define LITESYS_MPB_SAME_LEVEL          ((uintptr_t)1)
#define LITESYS_MPB_UP_LEVEL            ((uintptr_t)2)
#define LITESYS_MPB_DOWN_LEVEL          ((uintptr_t)4)
// direction
#define LITESYS_MPB_SELF                ((uintptr_t)8)
#define LITESYS_MPB_LEFT                ((uintptr_t)16)
#define LITESYS_MPB_RIGHT               ((uintptr_t)32)

// self level
#define LITESYS_MPB_SAME_UP_LEVEL       (LITESYS_MPB_SAME_LEVEL | LITESYS_MPB_UP_LEVEL)
#define LITESYS_MPB_SAME_DOWN_LEVEL     (LITESYS_MPB_SAME_LEVEL | LITESYS_MPB_DOWN_LEVEL)
#define LITESYS_MPB_ALL_LEVEL           (LITESYS_MPB_SAME_LEVEL | LITESYS_MPB_UP_LEVEL | LITESYS_MPB_DOWN_LEVEL)

// self direction
#define LITESYS_MPB_SELF_LEFT           (LITESYS_MPB_SELF | LITESYS_MPB_LEFT)
#define LITESYS_MPB_SELF_RIGHT          (LITESYS_MPB_SELF | LITESYS_MPB_RIGHT)
#define LITESYS_MPB_ALL_BLOCK           (LITESYS_MPB_SELF | LITESYS_MPB_LEFT | LITESYS_MPB_RIGHT)

// all
#define LITESYS_MPB_ALL                 (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_ALL_LEVEL)

// all level direction
#define LITESYS_MPB_ALL_SELF            (LITESYS_MPB_SELF | LITESYS_MPB_ALL_LEVEL)
#define LITESYS_MPB_ALL_LEFT            (LITESYS_MPB_LEFT | LITESYS_MPB_ALL_LEVEL)
#define LITESYS_MPB_ALL_RIGHT           (LITESYS_MPB_RIGHT | LITESYS_MPB_ALL_LEVEL)
#define LITESYS_MPB_ALL_SELF_LEFT       (LITESYS_MPB_SELF_LEFT | LITESYS_MPB_ALL_LEVEL)
#define LITESYS_MPB_ALL_SELF_RIGHT      (LITESYS_MPB_SELF_RIGHT | LITESYS_MPB_ALL_LEVEL)

// all direction level
#define LITESYS_MPB_ALL_SAME_LEVEL      (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_SAME_LEVEL)
#define LITESYS_MPB_ALL_UP_LEVEL        (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_UP_LEVEL)
#define LITESYS_MPB_ALL_DOWN_LEVEL      (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_DOWN_LEVEL)
#define LITESYS_MPB_ALL_SAME_UP_LEVEL   (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_SAME_UP_LEVEL)
#define LITESYS_MPB_ALL_SAME_DOWN_LEVEL (LITESYS_MPB_ALL_BLOCK | LITESYS_MPB_SAME_DOWN_LEVEL)

struct memory_pool_block_lite_t {
    memory_pool_block_lite *head;
    memory_pool_block_lite *prev;
    memory_pool_block_lite *next;
    uintptr_t               state;
};

struct memory_pool_block_t {
    void                   *start;
    uintptr_t               length;
    uintptr_t               remain;
    uintptr_t               level;
    memory_pool_block      *prev;
    memory_pool_block      *next;
    memory_pool_block_lite *mpb_table_last;
    memory_pool_block_lite  mpb_table;
};

#define GET_POS_AT_PARENT(XNAME, PTYPE) ((uintptr_t)(&(((PTYPE *)(0))->XNAME)))
#define GET_PARENT(PTR, NAME, PTYPE)    ((PTYPE *)(((uintptr_t)(PTR)) - (GET_POS_AT_PARENT(NAME, PTYPE))))

#ifdef __cplusplus
extern "C"
{
#endif

    memory_pool_block *memory_pool_init(void *ptr, uintptr_t size, uintptr_t level);
    void               memory_pool_deinit(memory_pool_block *mpb); // 将一个mpb销毁（不再可以使用且不会保留在链中）
    void              *memory_pool_malloc(memory_pool_block *mpb, uintptr_t size, uintptr_t mode, uintptr_t level);
    void               memory_pool_free(void *ptr);
    void              *memory_pool_realloc(void *ptr, uintptr_t size, uintptr_t mode);

    void               memory_pool_clear_free_allocated(memory_pool_block *mpb, uintptr_t mode, uintptr_t level); // 在mpb链中将level选定的mpb尽数释放内存 （推出的mpb在链中）
    void               memory_pool_clear_destory_heap(memory_pool_block *mpb, uintptr_t mode, uintptr_t level);   // 在mpb链中将level选定的mpb尽数销毁（推出的mpb不在链中但且不可以使用）
    void               memory_pool_clear_pop_heap(memory_pool_block *mpb, uintptr_t mode, uintptr_t level);       // 在mpb链中将level选定的mpb尽数推出（推出的mpb不在链中但仍然可以使用）

    memory_pool_block *memory_pool_heap_emplace_back(memory_pool_block *mpb, void *ptr, uintptr_t size, uintptr_t level); // 在一个mpb链的末尾建立一个mpb

    void               memory_pool_heap_connect(memory_pool_block *mpb, memory_pool_block *mpb2); // 将mpb2连接到mpb1的末尾

    int                memory_pool_heap_connect_at(memory_pool_block *mpb, memory_pool_block *mpb2); // 将mpb2连接到mpb1的位置（会在中间插入，如果插入一个nullptr，则会断开整个链）

    memory_pool_block *memory_pool_heap_pop_back(memory_pool_block *mpb); // 将mpb所指示的mpb链的最后一个元素弹出

    memory_pool_block *memory_pool_heap_pop_at(memory_pool_block *mpb); // 将mpb所指示的位置的mpb弹出

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <cstdio>
#include <cstdlib>
#include <new>

#endif

#endif /* MEMORY_POOL_BLOCK_MEMORY_POOL_BLOCK_HPP_ */

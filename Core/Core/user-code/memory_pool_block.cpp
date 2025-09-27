/*
 * memory_pool_block.cpp
 *
 *  Created on: Feb 20, 2025
 *      Author: DrL潇湘
 */


#include "memory_pool_block.hpp"
#include <utility>

memory_pool_block *memory_pool_init(void *ptr, uintptr_t size, uintptr_t level) {
    memory_pool_block *mpb = new (ptr) memory_pool_block;
    mpb->start             = ptr;
    mpb->length            = size;
    mpb->remain            = (size - MODULE_SIZE);
    mpb->prev              = nullptr;
    mpb->next              = nullptr;
    mpb->level             = level;
    mpb->mpb_table_last    = &(mpb->mpb_table);
    mpb->mpb_table.prev    = nullptr;
    mpb->mpb_table.next    = nullptr;
    mpb->mpb_table.state   = LITESYS_MPB;
    mpb->mpb_table.head    = &(mpb->mpb_table);

    return mpb;
}
void memory_pool_deinit(memory_pool_block *mpb) {
    if (!mpb) {
        return;
    }
    vTaskSuspendAll();

    mpb->start           = nullptr;
    mpb->length          = 0;
    mpb->remain          = 0;
    mpb->level           = 0;
    mpb->mpb_table_last  = &(mpb->mpb_table);
    mpb->mpb_table.state = LITESYS_MPB;

    if (mpb->prev)
        mpb->prev->next = mpb->next;
    if (mpb->next)
        mpb->next->prev = mpb->prev;
    if (mpb->mpb_table.prev)
        mpb->mpb_table.prev->next = mpb->mpb_table_last->next;
    if (mpb->mpb_table_last->next)
        mpb->mpb_table_last->next->prev = mpb->mpb_table.prev;

    mpb->prev           = nullptr;
    mpb->next           = nullptr;
    mpb->mpb_table.prev = nullptr;
    mpb->mpb_table.next = nullptr;

    xTaskResumeAll();
}

static void *memory_pool_malloc_perone(memory_pool_block *mpb, uintptr_t size) {
    if (mpb == nullptr)
        return nullptr;
    if (size == 0)
        return mpb->start;

    size_t                  aligned_size = ALIGN(size, sizeof(uintptr_t));
    memory_pool_block_lite *curr         = &(mpb->mpb_table);
    size_t                  remain_size  = mpb->length > MODULE_SIZE ? (mpb->length - MODULE_SIZE) : 0;
    void                   *ret          = nullptr;

    bool                    alloc        = false;

    while (curr != nullptr) {
        if (!(curr->state & LITESYS_ALLOC)) {
            size_t size_hold = 0;
            if (IS_LITE_LAST(curr)) {
                size_hold = remain_size;
            }
            else {
                size_hold = PTR_DIFF(curr, curr->next) - LITE_MODULE_SIZE;
            }
            if (size_hold >= size) {
                curr->state |= LITESYS_ALLOC;
                ret   = PTR_INC(curr, LITE_MODULE_SIZE);
                alloc = true;
                if (size_hold > (aligned_size + LITE_MODULE_SIZE)) {
                    void                   *construct_pos = PTR_INC(ret, aligned_size);
                    memory_pool_block_lite *new_mpbl      = new (construct_pos) memory_pool_block_lite;
                    new_mpbl->prev                        = curr;
                    new_mpbl->next                        = curr->next;
                    curr->next                            = new_mpbl;
                    if (new_mpbl->next)
                        new_mpbl->next->prev = new_mpbl;
                    new_mpbl->state = LITESYS_MPB_LITE;
                    new_mpbl->head  = &(mpb->mpb_table);
                    mpb->remain -= (aligned_size + LITE_MODULE_SIZE);
                    if (IS_LITE_LAST(new_mpbl)) {
                        mpb->mpb_table_last = new_mpbl;
                    }
                }
                else {
                    mpb->remain -= size_hold;
                }
            }
        }
        if (alloc)
            break;
        if (remain_size > PTR_DIFF(curr, curr->next))
            remain_size -= PTR_DIFF(curr, curr->next);
        else
            remain_size = 0;

        if (IS_LITE_LAST(curr))
            break;
        curr = curr->next;
    }

    return ret;
}

void *memory_pool_malloc(memory_pool_block *mpb, uintptr_t size, uintptr_t mode, uintptr_t level) {
    if (mpb == nullptr)
        return nullptr;
    void *ret = nullptr;

    vTaskSuspendAll();
    if (mode & LITESYS_MPB_SELF) {
        int l = (level == mpb->level) ? (0) : ((level > mpb->level) ? (1) : (2));
        switch (l) {
        case 0: {
            if (mode & LITESYS_MPB_SAME_LEVEL) {
                ret = memory_pool_malloc_perone(mpb, size);
                if (ret) {
                    xTaskResumeAll();
                    return ret;
                }
            }
            break;
        }
        case 1: {
            if (mode & LITESYS_MPB_DOWN_LEVEL) {
                ret = memory_pool_malloc_perone(mpb, size);
                if (ret) {
                    xTaskResumeAll();
                    return ret;
                }
            }
            break;
        }
        case 2: {
            if (mode & LITESYS_MPB_UP_LEVEL) {
                ret = memory_pool_malloc_perone(mpb, size);
                if (ret) {
                    xTaskResumeAll();
                    return ret;
                }
            }
            break;
        }
        default:
            break;
        }
    }
    if (mode & LITESYS_MPB_LEFT) {
        memory_pool_block *curr_mpb = mpb->prev;
        while (curr_mpb != nullptr) {
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            default:
                break;
            }
            curr_mpb = curr_mpb->prev;
        }
    }
    if (mode & LITESYS_MPB_RIGHT) {
        memory_pool_block *curr_mpb = mpb->next;
        while (curr_mpb != nullptr) {
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL) {
                    ret = memory_pool_malloc_perone(curr_mpb, size);
                    if (ret) {
                        xTaskResumeAll();
                        return ret;
                    }
                }
                break;
            }
            default:
                break;
            }
            curr_mpb = curr_mpb->next;
        }
    }

    xTaskResumeAll();
    return ret;
}

void memory_pool_free(void *ptr) {
    if (ptr == nullptr)
        return;

    vTaskSuspendAll();
    memory_pool_block_lite *mpbl   = (memory_pool_block_lite *)PTR_DEC(ptr, LITE_MODULE_SIZE);
    bool                    is_mpb = mpbl->state & LITESYS_MPB,
         fw_is_mpb                 = mpbl->next ? mpbl->next->state & LITESYS_MPB : true,
         fw_is_alloc               = mpbl->next ? mpbl->next->state & LITESYS_ALLOC : true,
         bw_is_alloc               = mpbl->prev ? mpbl->prev->state & LITESYS_ALLOC : true;
    bool backward_merge = false, forward_merge = false;
    backward_merge = !is_mpb && !bw_is_alloc;
    forward_merge  = !fw_is_mpb && !fw_is_alloc;

    mpbl->state &= ~(LITESYS_ALLOC);

    memory_pool_block      *mpb        = GET_PARENT(mpbl->head, mpb_table, memory_pool_block);
    memory_pool_block_lite *check_mpbl = mpbl;
    memory_pool_block_lite *next       = mpbl->next;

    if (!IS_LITE_LAST(mpbl))
        mpb->remain += PTR_DIFF(next, ptr);
    else
        mpb->remain += (mpb->length - PTR_DIFF(mpb, ptr));

    if (forward_merge) {
        if (!IS_LITE_LAST(next)) {
            mpb->remain += PTR_DIFF(next, next->next);
        }
        else {
            mpb->remain += (mpb->length - PTR_DIFF(mpb, next));
        }
        if (next->next)
            next->next->prev = mpbl;
        mpbl->next = next->next;
    }

    if (backward_merge) {
        memory_pool_block_lite *prev = mpbl->prev;
        prev->next                   = mpbl->next;
        if (mpbl->next)
            mpbl->next->prev = prev;
        mpb->remain += PTR_DIFF(prev, mpbl);

        check_mpbl = prev;
    }

    if (IS_LITE_LAST(check_mpbl)) {
        mpb->mpb_table_last = check_mpbl;
    }
    xTaskResumeAll();
}

void *memory_pool_realloc(void *ptr, uintptr_t size, uintptr_t mode) {
    if (ptr == nullptr)
        return nullptr;
    void *ret = nullptr;

    vTaskSuspendAll();
    memory_pool_block_lite *mpbl = (memory_pool_block_lite *)PTR_DEC(ptr, LITE_MODULE_SIZE);
    memory_pool_block      *mpb  = GET_PARENT(mpbl->head, mpb_table, memory_pool_block);
    // deal with
    if (ret) {
        xTaskResumeAll();
        return ret;
    }

    if (!((mode & LITESYS_MPB_SELF) && !(mode & (LITESYS_MPB_LEFT | LITESYS_MPB_RIGHT)))) {
        ret = memory_pool_malloc(mpb, size, mode, mpb->level);
        if (ret) {
            memory_pool_free(ret);
        }
    }

    xTaskResumeAll();
    return ret;
}

static void memory_pool_clear_perone(memory_pool_block *mpb) {
    if (mpb == nullptr)
        return;
    memory_pool_block *mpb_next = mpb->next;
    mpb->remain                 = (mpb->length - MODULE_SIZE);
    if (mpb_next)
        mpb->mpb_table.next = &(mpb_next->mpb_table);
    else
        mpb->mpb_table.next = nullptr;
    mpb->mpb_table.state &= (~LITESYS_ALLOC);
}
void memory_pool_clear_free_allocated(memory_pool_block *mpb, uintptr_t mode, uintptr_t level) {
    if (mpb == nullptr)
        return;

    vTaskSuspendAll();
    if (mode & LITESYS_MPB_SELF) {
        int l = (level == mpb->level) ? (0) : ((level > mpb->level) ? (1) : (2));
        switch (l) {
        case 0: {
            if (mode & LITESYS_MPB_SAME_LEVEL)
                memory_pool_clear_perone(mpb);
            break;
        }
        case 1: {
            if (mode & LITESYS_MPB_DOWN_LEVEL)
                memory_pool_clear_perone(mpb);
            break;
        }
        case 2: {
            if (mode & LITESYS_MPB_UP_LEVEL)
                memory_pool_clear_perone(mpb);
            break;
        }
        default:
            break;
        }
    }
    if (mode & LITESYS_MPB_LEFT) {
        memory_pool_block *curr_mpb = mpb->prev;
        while (curr_mpb != nullptr) {
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = curr_mpb->prev;
        }
    }
    if (mode & LITESYS_MPB_RIGHT) {
        memory_pool_block *curr_mpb = mpb->next;
        while (curr_mpb != nullptr) {
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_clear_perone(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = curr_mpb->next;
        }
    }
    xTaskResumeAll();
}
void memory_pool_clear_destory_heap(memory_pool_block *mpb, uintptr_t mode, uintptr_t level) {
    if (mpb == nullptr)
        return;

    vTaskSuspendAll();
    memory_pool_block *mpb_prev = mpb->prev;
    memory_pool_block *mpb_next = mpb->next;
    if (mode & LITESYS_MPB_SELF) {
        int l = (level == mpb->level) ? (0) : ((level > mpb->level) ? (1) : (2));
        switch (l) {
        case 0: {
            if (mode & LITESYS_MPB_SAME_LEVEL)
                memory_pool_deinit(mpb);
            break;
        }
        case 1: {
            if (mode & LITESYS_MPB_DOWN_LEVEL)
                memory_pool_deinit(mpb);
            break;
        }
        case 2: {
            if (mode & LITESYS_MPB_UP_LEVEL)
                memory_pool_deinit(mpb);
            break;
        }
        default:
            break;
        }
    }
    if (mode & LITESYS_MPB_LEFT) {
        memory_pool_block *curr_mpb = mpb_prev;
        memory_pool_block *prev;
        while (curr_mpb != nullptr) {
            prev  = curr_mpb->prev;
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = prev;
        }
    }
    if (mode & LITESYS_MPB_RIGHT) {
        memory_pool_block *curr_mpb = mpb_next;
        memory_pool_block *next;
        while (curr_mpb != nullptr) {
            next  = curr_mpb->next;
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_deinit(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = next;
        }
    }
    xTaskResumeAll();
}
void memory_pool_clear_pop_heap(memory_pool_block *mpb, uintptr_t mode, uintptr_t level) {
    if (mpb == nullptr)
        return;

    vTaskSuspendAll();
    memory_pool_block *mpb_prev = mpb->prev;
    memory_pool_block *mpb_next = mpb->next;
    if (mode & LITESYS_MPB_SELF) {
        int l = (level == mpb->level) ? (0) : ((level > mpb->level) ? (1) : (2));
        switch (l) {
        case 0: {
            if (mode & LITESYS_MPB_SAME_LEVEL)
                memory_pool_heap_pop_at(mpb);
            break;
        }
        case 1: {
            if (mode & LITESYS_MPB_DOWN_LEVEL)
                memory_pool_heap_pop_at(mpb);
            break;
        }
        case 2: {
            if (mode & LITESYS_MPB_UP_LEVEL)
                memory_pool_heap_pop_at(mpb);
            break;
        }
        default:
            break;
        }
    }
    if (mode & LITESYS_MPB_LEFT) {
        memory_pool_block *curr_mpb = mpb_prev;
        memory_pool_block *prev;
        while (curr_mpb != nullptr) {
            prev  = curr_mpb->prev;
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = prev;
        }
    }
    if (mode & LITESYS_MPB_RIGHT) {
        memory_pool_block *curr_mpb = mpb_next;
        memory_pool_block *next;
        while (curr_mpb != nullptr) {
            next  = curr_mpb->next;
            int l = (level == curr_mpb->level) ? (0) : ((level > curr_mpb->level) ? (1) : (2));
            switch (l) {
            case 0: {
                if (mode & LITESYS_MPB_SAME_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            case 1: {
                if (mode & LITESYS_MPB_DOWN_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            case 2: {
                if (mode & LITESYS_MPB_UP_LEVEL)
                    memory_pool_heap_pop_at(curr_mpb);
                break;
            }
            default:
                break;
            }
            curr_mpb = next;
        }
    }
    xTaskResumeAll();
}

memory_pool_block *memory_pool_heap_emplace_back(memory_pool_block *mpb, void *ptr, uintptr_t size, uintptr_t level) {
    vTaskSuspendAll();
    while (mpb->next != nullptr)
        mpb = mpb->next;
    memory_pool_block *new_mpb = new (ptr) memory_pool_block;
    new_mpb->start             = ptr;
    new_mpb->length            = size;
    new_mpb->remain            = (size - MODULE_SIZE);
    new_mpb->level             = level;
    new_mpb->prev              = mpb;
    new_mpb->next              = nullptr;
    new_mpb->mpb_table_last    = &(new_mpb->mpb_table);
    new_mpb->mpb_table.prev    = mpb->mpb_table_last;
    new_mpb->mpb_table.next    = nullptr;
    new_mpb->mpb_table.state   = LITESYS_MPB;
    new_mpb->mpb_table.head    = &(new_mpb->mpb_table);

    mpb->next                  = new_mpb;
    mpb->mpb_table_last->next  = &(new_mpb->mpb_table);

    xTaskResumeAll();
    return new_mpb;
}
void memory_pool_heap_connect(memory_pool_block *mpb, memory_pool_block *mpb2) {
    if (mpb == nullptr)
        return;
    if (mpb2 == nullptr)
        return;

    vTaskSuspendAll();
    while (mpb->next != nullptr)
        mpb = mpb->next;
    mpb->next                 = mpb2;
    mpb2->prev                = mpb;
    mpb->mpb_table_last->next = &(mpb2->mpb_table);
    mpb2->mpb_table.prev      = mpb->mpb_table_last;
    xTaskResumeAll();
}
int memory_pool_heap_connect_at(memory_pool_block *mpb, memory_pool_block *mpb2) {
    if (mpb == nullptr)
        return 0;

    vTaskSuspendAll();
    if (mpb2 && mpb2->prev != nullptr)
        return -1;

    memory_pool_block *mpb_prev = mpb->prev;

    if (mpb2 == nullptr) {
        if (mpb_prev == nullptr)
            return 0;
        mpb_prev->mpb_table_last->next = nullptr;
        mpb_prev->next                 = nullptr;
        mpb->mpb_table.prev            = nullptr;
        mpb->prev                      = nullptr;
        xTaskResumeAll();
        return 0;
    }

    memory_pool_block *mpb2_last = mpb2;
    while (mpb2_last->next != nullptr)
        mpb2_last = mpb2_last->next;
    if (mpb_prev != nullptr) {
        mpb_prev->next                 = mpb2;
        mpb2->prev                     = mpb_prev;
        mpb_prev->mpb_table_last->next = &(mpb2->mpb_table);
        mpb2->mpb_table.prev           = mpb_prev->mpb_table_last;
    }
    mpb->prev                       = mpb2_last;
    mpb2_last->next                 = mpb;
    mpb2_last->mpb_table_last->next = &(mpb->mpb_table);
    mpb->mpb_table.prev             = mpb2_last->mpb_table_last;
    xTaskResumeAll();

    return 0;
}
memory_pool_block *memory_pool_heap_pop_back(memory_pool_block *mpb) {
    vTaskSuspendAll();
    while (mpb->next != nullptr)
        mpb = mpb->next;
    memory_pool_block *ret = memory_pool_heap_pop_at(mpb);
    xTaskResumeAll();
    return ret;
}
memory_pool_block *memory_pool_heap_pop_at(memory_pool_block *mpb) {
    if (mpb == nullptr)
        return nullptr;

    vTaskSuspendAll();
    memory_pool_block *ret      = mpb->next;
    memory_pool_block *mpb_prev = mpb->prev;

    mpb->next                   = nullptr;
    mpb->prev                   = nullptr;
    mpb->mpb_table.prev         = nullptr;
    mpb->mpb_table_last->next   = nullptr;

    if (ret != nullptr) {
        ret->prev           = mpb_prev;
        ret->mpb_table.prev = mpb_prev ? mpb_prev->mpb_table_last : nullptr;
    }

    if (mpb_prev != nullptr) {
        mpb_prev->next                 = ret;
        mpb_prev->mpb_table_last->next = ret ? &(ret->mpb_table) : nullptr;
    }

    xTaskResumeAll();
    return ret;
}

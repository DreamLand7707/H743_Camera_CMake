/*
 * my_heap_manage.c
 *
 *  Created on: Dec 5, 2024
 *      Author: DrL潇湘
 */

#ifndef INC_MY_HEAP_MANAGE_H_
#define INC_MY_HEAP_MANAGE_H_

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C"
{
#endif
    void* 		sdram_Malloc(size_t xWantedSize);
    void 		sdram_Free(void *pv);
    size_t 		sdram_GetFreeHeapSize(void);
    size_t 		sdram_GetMinimumEverFreeHeapSize(void);
    void 		sdram_InitialiseBlocks(void);
    void 		sdram_GetHeapStats(HeapStats_t *pxHeapStats);
#ifdef __cplusplus
}
#endif



#endif /* INC_MY_HEAP_MANAGE_H_ */

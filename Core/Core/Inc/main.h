/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
#include "lvgl.h"
#include "ansi.h"
#include "debugger.h"
#include "my_heap_manage.h"
#include <stdint.h>
    /* USER CODE END Includes */

    /* Exported types ------------------------------------------------------------*/
    /* USER CODE BEGIN ET */

#define jprintf SEGGER_RTT_printf

    typedef struct touch_point_ {
        uint32_t x;
        uint32_t y;
        uint32_t size;
        uint32_t state;
    } touch_point;

    typedef union __packed BGR_
    {
        struct {
            uint16_t B : 5;
            uint16_t G : 6;
            uint16_t R : 5;
        };
        uint16_t value;
    } BGR;

    typedef void (*touch_ic_isr)();

    /* USER CODE END ET */

    /* Exported constants --------------------------------------------------------*/
    /* USER CODE BEGIN EC */
    extern uint8_t           sram2_end;
    extern uint8_t           sram3_end;
    extern uint8_t           sdram_start;
    extern uint8_t           sdram_end;
    extern void *const       sram2_end_ptr;
    extern void *const       sram3_end_ptr;
    extern void *const       sdram_start_ptr;
    extern void *const       sdram_end_ptr;

    extern osThreadId        Initial_TaskHandle;
    extern osThreadId        FlashScr_TaskHandle;
    extern osThreadId        Camera_TaskHandle;

    extern SemaphoreHandle_t sema_flash_screen_routine_start;
    extern SemaphoreHandle_t sema_camera_routine_start;
    extern SemaphoreHandle_t sema_swap_buffer_handle;
    extern SemaphoreHandle_t sema_camera_routine_init_done;
    extern uint8_t           mkfs_buffer[4096] __attribute__((section(".fatfs_buffer")));

    extern touch_point       record_touch[10];
    extern lv_display_t     *rgb_screen_disp;
    extern uint32_t          sdcard_link_driver;
    extern uint32_t          sdcard_is_mounted;

    extern uint8_t          *file_exchange_buffer;
    extern uint8_t          *jpeg_after_buffer;      // YCbCr
    extern uint8_t          *jpeg_before_buffer_rgb; // YCbCr -> RGB
    extern uint8_t          *full_screen_pict_show_buffer;
    extern uint32_t          full_pict_show_size[2];
    extern uint8_t          *widget_pict_show_buffer;
    extern uint32_t          widget_pict_show_size[2];

    extern lv_obj_t         *file_explorer_main_screen;
    extern lv_obj_t         *full_screen_pict_screen;
    extern lv_obj_t         *camera_screen;

    // extern traceString       trace_analyzer_channel1;
    // extern traceString       trace_analyzer_channel2;
    // extern traceString       trace_analyzer_channel3;
    // extern traceString       trace_analyzer_channel4;

    /* USER CODE END EC */

    /* Exported macro ------------------------------------------------------------*/
    /* USER CODE BEGIN EM */

    /* USER CODE END EM */

    /* Exported functions prototypes ---------------------------------------------*/
    void
    Error_Handler(void);

    /* USER CODE BEGIN EFP */
    int  __io_putchar(int ch);
    int  __io_getchar(void);

    void print_sdcard_info(void);
    void initial_task_routine(void const *argument);
    void touch_sence_routine(void const *argument);
    void camera_task_routine(void const *argument);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define T_CS_Pin                       GPIO_PIN_8
#define T_CS_GPIO_Port                 GPIOI
#define KEY2_Pin                       GPIO_PIN_13
#define KEY2_GPIO_Port                 GPIOC
#define WK_UP_Pin                      GPIO_PIN_0
#define WK_UP_GPIO_Port                GPIOA
#define KEY1_Pin                       GPIO_PIN_2
#define KEY1_GPIO_Port                 GPIOH
#define KEY0_Pin                       GPIO_PIN_3
#define KEY0_GPIO_Port                 GPIOH
#define LED1_Pin                       GPIO_PIN_0
#define LED1_GPIO_Port                 GPIOB
#define LED0_Pin                       GPIO_PIN_1
#define LED0_GPIO_Port                 GPIOB
#define T_SCK_Pin                      GPIO_PIN_6
#define T_SCK_GPIO_Port                GPIOH
#define T_PEN_Pin                      GPIO_PIN_7
#define T_PEN_GPIO_Port                GPIOH
#define DCMI_XCLK_Pin                  GPIO_PIN_8
#define DCMI_XCLK_GPIO_Port            GPIOA
#define T_SDA_Pin                      GPIO_PIN_3
#define T_SDA_GPIO_Port                GPIOI
#define DCMI_RESET_Pin                 GPIO_PIN_15
#define DCMI_RESET_GPIO_Port           GPIOA
#define DCMI_SDA_Pin                   GPIO_PIN_3
#define DCMI_SDA_GPIO_Port             GPIOB
#define DCMI_SCL_Pin                   GPIO_PIN_4
#define DCMI_SCL_GPIO_Port             GPIOB
#define LCD_BL_Pin                     GPIO_PIN_5
#define LCD_BL_GPIO_Port               GPIOB

    /* USER CODE BEGIN Private defines */

#define IN_SDRAM                       __attribute__((section(".sdram")))
#define IN_SRAM2                       __attribute__((section(".ram_d2")))
#define IN_SRAM3                       __attribute__((section(".ram_d3")))

#define CLOSE_BITS(TARGET, MASK)       ((TARGET) &= (~(MASK)))
#define UNSET_BITS(TARGET, MASK)       ((TRAGET) &= (~(MASK)))
#define RESET_BITS(TARGET, MASK)       ((TRAGET) &= (~(MASK)))
#define OPEN_BITS(TARGET, MASK)        ((TARGET) |= (MASK))
#define SET_BITS(TARGET, MASK)         ((TARGET) |= (MASK))
#define SWITCH_BITS(TARGET, MASK)      ((TARGET) ^= (MASK))
#define WARP_BITS(TARGET, MASK)        ((TARGET) &= (MASK))
#define EXTRACT_BITS(TARGET, MASK)     ((TARGET) &= (MASK))

#define CLOSE_BITS_GET(TARGET, MASK)   ((TARGET) & (~(MASK)))
#define UNSET_BITS_GET(TARGET, MASK)   ((TARGET) & (~(MASK)))
#define RESET_BITS_GET(TARGET, MASK)   ((TARGET) & (~(MASK)))
#define OPEN_BITS_GET(TARGET, MASK)    ((TARGET) | (MASK))
#define SET_BITS_GET(TARGET, MASK)     ((TARGET) | (MASK))
#define SWITCH_BITS_GET(TARGET, MASK)  ((TARGET) ^ (MASK))
#define WARP_BITS_GET(TARGET, MASK)    ((TARGET) & (MASK))
#define EXTRACT_BITS_GET(TARGET, MASK) ((TARGET) & (MASK))

#define USE_CACHE_ID                   1

#ifdef DEBUG
    #define ADD_SEMAQUEUE(SEMAQUEUE, NAME) vQueueAddToRegistry(SEMAQUEUE, #NAME)
    #define DEL_SEMAQUEUE(SEMAQUEUE)       vQueueUnregisterQueue(SEMAQUEUE)
#else
    #define ADD_SEMAQUEUE(SEMAQUEUE, NAME)
    #define DEL_SEMAQUEUE(SEMAQUEUE)
#endif

#if USE_CACHE_ID == 1
    static inline void MYSCB_CleanInvalidateDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
        uint32_t address_start = (uint32_t)pData_begin;
        uint32_t address_end   = (uint32_t)pData_end + 31;
        address_start &= 0xffffffe0;
        address_end &= 0xffffffe0;
        int32_t real_size = (int32_t)(address_end - address_start);
        SCB_CleanInvalidateDCache_by_Addr((uint32_t *)address_start, real_size);
    }

    static inline void MYSCB_CleanDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
        uint32_t address_start = (uint32_t)pData_begin;
        uint32_t address_end   = (uint32_t)pData_end + 31;
        address_start &= 0xffffffe0;
        address_end &= 0xffffffe0;
        int32_t real_size = (int32_t)(address_end - address_start);
        SCB_CleanDCache_by_Addr((uint32_t *)address_start, real_size);
    }

    static inline void MYSCB_InvalidateDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
        uint32_t address_start = (uint32_t)pData_begin;
        uint32_t address_end   = (uint32_t)pData_end + 31;
        address_start &= 0xffffffe0;
        address_end &= 0xffffffe0;
        int32_t real_size = (int32_t)(address_end - address_start);
        SCB_InvalidateDCache_by_Addr((uint32_t *)address_start, real_size);
    }
#else
static inline void MYSCB_CleanInvalidateDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
    UNUSED(pData_begin);
    UNUSED(pData_end);
}

static inline void MYSCB_CleanDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
    UNUSED(pData_begin);
    UNUSED(pData_end);
}

static inline void MYSCB_InvalidateDCache_by_AddrRange(const void *pData_begin, const void *pData_end) {
    UNUSED(pData_begin);
    UNUSED(pData_end);
}
#endif

    static inline int rgba_equal(BGR *a, BGR *b) {
        return (a->B == b->B) && (a->G == b->G) && (a->R == b->R);
    }

    __STATIC_FORCEINLINE int in_handler_mode(void) {
        return __get_IPSR() != 0;
    }

    /* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

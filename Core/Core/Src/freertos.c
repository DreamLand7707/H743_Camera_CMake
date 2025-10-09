/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "fatfs.h"
#include "memorymap.h"
#include "sdmmc.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include "tim.h"
#include "lvgl.h"
#include "lvgl/examples/porting/lv_port_disp.h"
#include "lvgl/examples/porting/lv_port_indev.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId Initial_TaskHandle;
osThreadId TouchSence_TaskHandle;
osThreadId Camera_TaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void) {
    HAL_TIM_Base_Start_IT(&htim3);
}
extern volatile unsigned long ulHighFrequencyTimerTicks;
__weak unsigned long getRunTimeCounterValue(void) {
    return ulHighFrequencyTimerTicks;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 2 */
__weak void vApplicationIdleHook(void) {
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
     to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
     task. It is essential that code added to this hook function never attempts
     to block in any way (for example, call xQueueReceive() with a block time
     specified, or call vTaskDelay()). If the application makes use of the
     vTaskDelete() API function (as this demo application does) then it is also
     important that vApplicationIdleHook() is permitted to return to its calling
     function, because it is the responsibility of the idle task to clean up
     memory allocated by the kernel to any task that has since been deleted. */
}
/* USER CODE END 2 */

/* USER CODE BEGIN 3 */
__weak void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
    lv_tick_inc(1);
}
/* USER CODE END 3 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    sema_flash_screen_routine_start = xSemaphoreCreateBinary();
    sema_camera_routine_start = xSemaphoreCreateBinary();
    sema_swap_buffer_handle = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */

  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of Initial_Task */
  osThreadDef(Initial_Task, initial_task_routine, osPriorityAboveNormal, 0, 2048);
  Initial_TaskHandle = osThreadCreate(osThread(Initial_Task), NULL);

  /* definition and creation of TouchSence_Task */
  osThreadDef(TouchSence_Task, touch_sence_routine, osPriorityNormal, 0, 2048);
  TouchSence_TaskHandle = osThreadCreate(osThread(TouchSence_Task), NULL);

  /* definition and creation of Camera_Task */
  osThreadDef(Camera_Task, camera_task_routine, osPriorityNormal, 0, 4096);
  Camera_TaskHandle = osThreadCreate(osThread(Camera_Task), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    vQueueAddToRegistry((QueueHandle_t) sema_flash_screen_routine_start, "sema_flash_screen_routine_start");
    vQueueAddToRegistry((QueueHandle_t) sema_camera_routine_start, "sema_camera_routine_start");
    vQueueAddToRegistry((QueueHandle_t) sema_swap_buffer_handle, "sema_swap_buffer_handle");
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_initial_task_routine */
/**
 * @brief  Function implementing the Initial_Task thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_initial_task_routine */
__weak void initial_task_routine(void const * argument)
{
  /* USER CODE BEGIN initial_task_routine */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
  /* USER CODE END initial_task_routine */
}

/* USER CODE BEGIN Header_touch_sence_routine */
/**
* @brief Function implementing the TouchSence_Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_touch_sence_routine */
__weak void touch_sence_routine(void const * argument)
{
  /* USER CODE BEGIN touch_sence_routine */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END touch_sence_routine */
}

/* USER CODE BEGIN Header_camera_task_routine */
/**
 * @brief Function implementing the Camera_Task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_camera_task_routine */
__weak void camera_task_routine(void const * argument)
{
  /* USER CODE BEGIN camera_task_routine */
    /* Infinite loop */
    for (;;) {
        osDelay(1);
    }
  /* USER CODE END camera_task_routine */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

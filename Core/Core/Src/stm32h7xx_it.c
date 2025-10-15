/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32h7xx_it.c
 * @brief   Interrupt Service Routines.
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
#include "main.h"
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern volatile unsigned long ulHighFrequencyTimerTicks;
volatile unsigned long        ulHighFrequencyTimerTicks = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef   hdma_dcmi;
extern DMA2D_HandleTypeDef hdma2d;
extern SDRAM_HandleTypeDef hsdram1;
extern MDMA_HandleTypeDef  hmdma_jpeg_infifo_th;
extern MDMA_HandleTypeDef  hmdma_jpeg_outfifo_th;
extern JPEG_HandleTypeDef  hjpeg;
extern LTDC_HandleTypeDef  hltdc;
extern SD_HandleTypeDef    hsd2;
extern TIM_HandleTypeDef   htim3;
extern TIM_HandleTypeDef   htim7;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
 * @brief This function handles Non maskable interrupt.
 */
void NMI_Handler(void) {
    /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

    /* USER CODE END NonMaskableInt_IRQn 0 */
    /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
    while (1) {
    }
    /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
 * @brief This function handles Hard fault interrupt.
 */
void HardFault_Handler(void) {
    /* USER CODE BEGIN HardFault_IRQn 0 */

    /* USER CODE END HardFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_HardFault_IRQn 0 */
        /* USER CODE END W1_HardFault_IRQn 0 */
    }
}

/**
 * @brief This function handles Memory management fault.
 */
void MemManage_Handler(void) {
    /* USER CODE BEGIN MemoryManagement_IRQn 0 */

    /* USER CODE END MemoryManagement_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
        /* USER CODE END W1_MemoryManagement_IRQn 0 */
    }
}

/**
 * @brief This function handles Pre-fetch fault, memory access fault.
 */
void BusFault_Handler(void) {
    /* USER CODE BEGIN BusFault_IRQn 0 */

    /* USER CODE END BusFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_BusFault_IRQn 0 */
        /* USER CODE END W1_BusFault_IRQn 0 */
    }
}

/**
 * @brief This function handles Undefined instruction or illegal state.
 */
void UsageFault_Handler(void) {
    /* USER CODE BEGIN UsageFault_IRQn 0 */

    /* USER CODE END UsageFault_IRQn 0 */
    while (1) {
        /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
        /* USER CODE END W1_UsageFault_IRQn 0 */
    }
}

/**
 * @brief This function handles Debug monitor.
 */
void DebugMon_Handler(void) {
    /* USER CODE BEGIN DebugMonitor_IRQn 0 */

    /* USER CODE END DebugMonitor_IRQn 0 */
    /* USER CODE BEGIN DebugMonitor_IRQn 1 */

    /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
 * @brief This function handles DMA1 stream0 global interrupt.
 */

/**
 * @brief This function handles TIM3 global interrupt.
 */
void TIM3_IRQHandler(void) {
    /* USER CODE BEGIN TIM3_IRQn 0 */

    /* USER CODE END TIM3_IRQn 0 */
    HAL_TIM_IRQHandler(&htim3);
    /* USER CODE BEGIN TIM3_IRQn 1 */
    ulHighFrequencyTimerTicks++;
    /* USER CODE END TIM3_IRQn 1 */
}

/**
 * @brief This function handles FMC global interrupt.
 */
void FMC_IRQHandler(void) {
    /* USER CODE BEGIN FMC_IRQn 0 */

    /* USER CODE END FMC_IRQn 0 */
    HAL_SDRAM_IRQHandler(&hsdram1);
    /* USER CODE BEGIN FMC_IRQn 1 */

    /* USER CODE END FMC_IRQn 1 */
}

/**
 * @brief This function handles SDMMC1 global interrupt.
 */
void SDMMC2_IRQHandler(void) {
    /* USER CODE BEGIN SDMMC1_IRQn 0 */

    /* USER CODE END SDMMC1_IRQn 0 */
    HAL_SD_IRQHandler(&hsd2);
    /* USER CODE BEGIN SDMMC1_IRQn 1 */

    /* USER CODE END SDMMC1_IRQn 1 */
}

/**
 * @brief This function handles TIM7 global interrupt.
 */
void TIM7_IRQHandler(void) {
    /* USER CODE BEGIN TIM7_IRQn 0 */
    __HAL_TIM_DISABLE(&htim7);
    /* USER CODE END TIM7_IRQn 0 */
    HAL_TIM_IRQHandler(&htim7);
    /* USER CODE BEGIN TIM7_IRQn 1 */
    __HAL_TIM_ENABLE(&htim7);
    /* USER CODE END TIM7_IRQn 1 */
}

/**
 * @brief This function handles LTDC global interrupt.
 */
void LTDC_IRQHandler(void) {
    /* USER CODE BEGIN LTDC_IRQn 0 */

    /* USER CODE END LTDC_IRQn 0 */
    HAL_LTDC_IRQHandler(&hltdc);
    /* USER CODE BEGIN LTDC_IRQn 1 */

    /* USER CODE END LTDC_IRQn 1 */
}

/**
 * @brief This function handles DMA2D global interrupt.
 */
void DMA2D_IRQHandler(void) {
    /* USER CODE BEGIN DMA2D_IRQn 0 */

    /* USER CODE END DMA2D_IRQn 0 */
    HAL_DMA2D_IRQHandler(&hdma2d);
    /* USER CODE BEGIN DMA2D_IRQn 1 */

    /* USER CODE END DMA2D_IRQn 1 */
}

/**
 * @brief This function handles DMAMUX1 overrun interrupt.
 */
void DMAMUX1_OVR_IRQHandler(void) {
    /* USER CODE BEGIN DMAMUX1_OVR_IRQn 0 */

    /* USER CODE END DMAMUX1_OVR_IRQn 0 */
    /* USER CODE BEGIN DMAMUX1_OVR_IRQn 1 */

    /* USER CODE END DMAMUX1_OVR_IRQn 1 */
}

/**
 * @brief This function handles JPEG global interrupt.
 */
void JPEG_IRQHandler(void) {
    /* USER CODE BEGIN JPEG_IRQn 0 */

    /* USER CODE END JPEG_IRQn 0 */
    HAL_JPEG_IRQHandler(&hjpeg);
    /* USER CODE BEGIN JPEG_IRQn 1 */

    /* USER CODE END JPEG_IRQn 1 */
}

/**
 * @brief This function handles MDMA global interrupt.
 */
void MDMA_IRQHandler(void) {
    /* USER CODE BEGIN MDMA_IRQn 0 */

    /* USER CODE END MDMA_IRQn 0 */
    HAL_MDMA_IRQHandler(&hmdma_jpeg_infifo_th);
    HAL_MDMA_IRQHandler(&hmdma_jpeg_outfifo_th);
    /* USER CODE BEGIN MDMA_IRQn 1 */
    camera_mdma_IRQHandler();
    /* USER CODE END MDMA_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

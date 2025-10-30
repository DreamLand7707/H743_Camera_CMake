#define FILE_DEBUG 1

#include "camera_declare.hpp"


// filtered IRQ Handler

void camera_RGB_YCbCr_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] %u tick: camera_RGB_YCbCr_DMA_Cplt_Cb\n", xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_CPLT, &should_yield);
    xQueueSendFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] %u tick: camera_RGB_YCbCr_DMA_M1_Cplt_Cb\n", xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_M1_CPLT, &should_yield);
    xQueueSendFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_DMA_Error_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] %u tick: camera_RGB_YCbCr_DMA_Error_Cb, ErrorCode: %u\n" ANSI_COLOR_RESET,
          xTaskGetTickCountFromISR(), hdma->ErrorCode);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_DMA_Abort_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] %u tick: camera_RGB_YCbCr_DMA_Abort_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_GREEN "[ISR] %u tick: camera_RGB_YCbCr_MDMA_Cplt_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug("[ISR] %u tick: camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb\n", xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_REPEAT_CPLT, &should_yield);
    xQueueReceiveFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] %u tick: camera_RGB_YCbCr_MDMA_Error_Cb, ErrorCode: %u\n" ANSI_COLOR_RESET,
          xTaskGetTickCountFromISR(), hmdma->ErrorCode);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_RGB_YCbCr_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] %u tick: camera_RGB_YCbCr_MDMA_Abort_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}



void camera_JPEG_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] %u tick: camera_JPEG_DMA_Cplt_Cb\n", xTaskGetTickCountFromISR());

    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_CPLT, &should_yield);
    xQueueSendFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);

    p->jpeg_data_count_calculate += p->half_middle_buffer_words;
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] %u tick: camera_JPEG_DMA_M1_Cplt_Cb\n", xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_M1_CPLT, &should_yield);
    xQueueSendFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);

    p->jpeg_data_count_calculate += p->half_middle_buffer_words;
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_DMA_Error_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] %u tick: camera_JPEG_DMA_Error_Cb, ErrorCode: %u\n" ANSI_COLOR_RESET,
          xTaskGetTickCountFromISR(), hdma->ErrorCode);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_DMA_Abort_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] %u tick: camera_JPEG_DMA_Abort_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_GREEN "[ISR] %u tick: camera_JPEG_MDMA_Cplt_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug("[ISR] %u tick: camera_JPEG_MDMA_RepeatBlock_Cplt_Cb\n", xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_REPEAT_CPLT, &should_yield);
    xQueueReceiveFromISR(p->MDMA_sync, nullptr, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);

    if (p->jpeg_mode) {
        if (p->jpeg_sw_final) {
            xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_CPLT, &should_yield);
            xSemaphoreGiveFromISR(camera_new_message, &should_yield);
        }
    }

    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] %u tick: camera_JPEG_MDMA_Error_Cb, ErrorCode: %u\n" ANSI_COLOR_RESET,
          xTaskGetTickCountFromISR(), hmdma->ErrorCode);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void camera_JPEG_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] %u tick: camera_JPEG_MDMA_Abort_Cb\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}




extern "C" void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    Camera_DCMI_Data *p = container_of(hdcmi, Camera_DCMI_Data, instance);
    //
    debug(ANSI_COLOR_FG_YELLOW "[ISR] %u tick: HAL_DCMI_FrameEventCallback\n" ANSI_COLOR_RESET, xTaskGetTickCountFromISR());
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, CAMERA_CAPTURE_OK, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

extern "C" void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi) {
    Camera_DCMI_Data *p = container_of(hdcmi, Camera_DCMI_Data, instance);
    //
    debug("[ISR] %u tick: HAL_DCMI_ErrorCallback, ErrorCode: %u\n",
          xTaskGetTickCountFromISR(),
          (unsigned)hdcmi->ErrorCode);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, CAMERA_CAPTURE_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}


// middle IRQ Handler

void camera_mdma_IRQHandler() {
    if (target_dcmi_is_ok) {
        HAL_MDMA_IRQHandler(&target_dcmi->second_stage_dma);
    }
}

void MY_HAL_DCMI_IRQHandler(DCMI_HandleTypeDef *hdcmi) {
    uint32_t isr_value = READ_REG(hdcmi->Instance->MISR);

    /* Synchronization error interrupt management *******************************/
    if ((isr_value & DCMI_FLAG_ERRRI) == DCMI_FLAG_ERRRI) {
        /* Clear the Synchronization error flag */
        __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_ERRRI);

        /* Update error code */
        hdcmi->ErrorCode |= HAL_DCMI_ERROR_SYNC;

        /* Change DCMI state */
        hdcmi->State = HAL_DCMI_STATE_ERROR;

        /* Set the synchronization error callback */
        hdcmi->ErrorCallback(hdcmi);
    }
    /* Overflow interrupt management ********************************************/
    if ((isr_value & DCMI_FLAG_OVRRI) == DCMI_FLAG_OVRRI) {
        /* Clear the Overflow flag */
        __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_OVRRI);

        /* Update error code */
        hdcmi->ErrorCode |= HAL_DCMI_ERROR_OVR;

        /* Change DCMI state */
        hdcmi->State = HAL_DCMI_STATE_ERROR;

        /* Set the overflow callback */
        hdcmi->ErrorCallback(hdcmi);
    }
    /* Line Interrupt management ************************************************/
    if ((isr_value & DCMI_FLAG_LINERI) == DCMI_FLAG_LINERI) {
        /* Clear the Line interrupt flag */
        __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_LINERI);

        /* Line interrupt Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
        /*Call registered DCMI line event callback*/
        hdcmi->LineEventCallback(hdcmi);
#else
        HAL_DCMI_LineEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
    }
    /* VSYNC interrupt management ***********************************************/
    if ((isr_value & DCMI_FLAG_VSYNCRI) == DCMI_FLAG_VSYNCRI) {
        /* Clear the VSYNC flag */
        __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_VSYNCRI);

        /* VSYNC Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
        /*Call registered DCMI vsync event callback*/
        hdcmi->VsyncEventCallback(hdcmi);
#else
        HAL_DCMI_VsyncEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
    }
    /* FRAME interrupt management ***********************************************/
    if ((isr_value & DCMI_FLAG_FRAMERI) == DCMI_FLAG_FRAMERI) {
        /* When snapshot mode, disable Vsync, Error and Overrun interrupts */
        if ((hdcmi->Instance->CR & DCMI_CR_CM) == DCMI_MODE_SNAPSHOT) {
            /* Disable the Line, Vsync, Error and Overrun interrupts */
            __HAL_DCMI_DISABLE_IT(hdcmi, DCMI_IT_LINE | DCMI_IT_VSYNC | DCMI_IT_ERR | DCMI_IT_OVR);
        }

        /* Disable the Frame interrupt */
        __HAL_DCMI_DISABLE_IT(hdcmi, DCMI_IT_FRAME);

        /* Clear the End of Frame flag */
        __HAL_DCMI_CLEAR_FLAG(hdcmi, DCMI_FLAG_FRAMERI);

        /* Frame Callback */
#if (USE_HAL_DCMI_REGISTER_CALLBACKS == 1)
        /*Call registered DCMI frame event callback*/
        hdcmi->FrameEventCallback(hdcmi);
#else
        HAL_DCMI_FrameEventCallback(hdcmi);
#endif /* USE_HAL_DCMI_REGISTER_CALLBACKS */
    }
}


// original IRQ Handler

extern "C" void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&(target_dcmi->first_stage_dma));
}

extern "C" void DCMI_IRQHandler(void) {
    MY_HAL_DCMI_IRQHandler(&(target_dcmi->instance));
}

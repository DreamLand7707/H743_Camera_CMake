#define FILE_DEBUG 1

#include "camera_declare.hpp"


SemaphoreHandle_t camera_new_message {};
SemaphoreHandle_t camera_exit {};
SemaphoreHandle_t camera_error {};
SemaphoreHandle_t camera_take_photo {};
QueueSetHandle_t  camera_queue_set {};

void              dcmi_capture_resource_init() {
    camera_queue_set         = xQueueCreateSet(6);
    camera_interface_changed = xSemaphoreCreateBinary();
    camera_new_message       = xSemaphoreCreateBinary();
    camera_exit              = xSemaphoreCreateBinary();
    camera_error             = xSemaphoreCreateBinary();
    camera_interface_restart = xSemaphoreCreateBinary();
    camera_take_photo        = xSemaphoreCreateBinary();

    xQueueAddToSet(camera_new_message, camera_queue_set);
    xQueueAddToSet(camera_exit, camera_queue_set);
    xQueueAddToSet(camera_error, camera_queue_set);
    xQueueAddToSet(camera_interface_changed, camera_queue_set);
    xQueueAddToSet(camera_interface_restart, camera_queue_set);
    xQueueAddToSet(camera_take_photo, camera_queue_set);
}

HAL_StatusTypeDef camera_start_capture(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format,
                                       uintptr_t middle_buffer, size_t middle_buffer_len,
                                       uintptr_t final_buffer, size_t final_buffer_len) {
    __HAL_LOCK(&Camera_DCMI->instance);

    Camera_DCMI->instance.State = HAL_DCMI_STATE_BUSY;

    __HAL_DCMI_ENABLE(&Camera_DCMI->instance);

    Camera_DCMI->instance.Instance->CR &= ~(DCMI_CR_CM);
    Camera_DCMI->instance.Instance->CR |= (uint32_t)(DCMI_MODE_SNAPSHOT);

    switch (target_format) {
    case camera_format::format_RGB:
    case camera_format::format_YCbCr: {

        configASSERT(middle_buffer % 4 == 0);
        configASSERT(middle_buffer_len % 32 == 0);
        configASSERT(final_buffer % 4 == 0);
        configASSERT(final_buffer_len % 32 == 0);

        size_t       middle_buffer_words         = middle_buffer_len >> 2u;
        size_t       final_buffer_words          = final_buffer_len >> 2u;

        const size_t mdma_single_block_max_words = 65536 / 4;

        bool         single_buffer_is_fine =
            (middle_buffer_words >= final_buffer_words) && (final_buffer_words <= mdma_single_block_max_words);

        Camera_DCMI->single_buffer_fine = single_buffer_is_fine;
        Camera_DCMI->jpeg_mode          = false;
        if (single_buffer_is_fine) { // single buffer is fine
            Camera_DCMI->double_buffer_first          = middle_buffer;
            Camera_DCMI->double_buffer_second         = 0;
            Camera_DCMI->half_middle_buffer_words     = final_buffer_words;
            Camera_DCMI->double_buffer_transmit_count = 1;
            Camera_DCMI->per_block_word               = final_buffer_words;
            Camera_DCMI->block_count                  = 1;
            Camera_DCMI->final_buffer                 = final_buffer;
            Camera_DCMI->final_buffer_len             = final_buffer_len;
            Camera_DCMI->dma_not_full                 = false;
            Camera_DCMI->jpeg_data_count_calculate    = 0;

            debug("Double Buffer Transmit Count: %u\n", 1);

            HAL_MDMA_DeInit(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_Init(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_REPBLOCKCPLT_CB_ID, camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_MDMA_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_MDMA_Error_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_MDMA_Abort_Cb);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            Camera_DCMI->mdma_first_buffer = true;
            HAL_MDMA_Start_IT(&Camera_DCMI->second_stage_dma,
                              (uint32_t)(Camera_DCMI->double_buffer_first),
                              (uint32_t)(Camera_DCMI->final_buffer),
                              (uint32_t)(Camera_DCMI->per_block_word << 2u),
                              (uint32_t)(Camera_DCMI->block_count));

            taskENTER_CRITICAL();
            {
                target_dcmi_is_ok = true;
            }
            taskEXIT_CRITICAL();

            // start dma and mdma
            HAL_DMA_DeInit(&(Camera_DCMI->first_stage_dma));
            HAL_DMA_Init(&(Camera_DCMI->first_stage_dma));
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_DMA_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_M1CPLT_CB_ID, camera_RGB_YCbCr_DMA_M1_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_DMA_Error_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_DMA_Abort_Cb);
            HAL_DMA_Start_IT(&(Camera_DCMI->first_stage_dma),
                             (uint32_t)&(Camera_DCMI->instance.Instance->DR),
                             (uint32_t)(Camera_DCMI->double_buffer_first),
                             (uint32_t)(Camera_DCMI->half_middle_buffer_words));
        }
        else { // double buffer!
            size_t half_middle_buffer_words = (middle_buffer_words >> 1u) > 0xffff ? 0xffff : (middle_buffer_words >> 1u);
            size_t per_block_word = 0, block_count = 0;
            calculate_decompose(half_middle_buffer_words, per_block_word, block_count, mdma_single_block_max_words);

            uintptr_t double_buffer_first          = middle_buffer;
            uintptr_t double_buffer_second         = middle_buffer + (half_middle_buffer_words << 2);
            size_t    double_buffer_transmit_count = divide_ceil(final_buffer_words, half_middle_buffer_words);
            bool      dma_not_full                 = (final_buffer_words % half_middle_buffer_words);

            //
            Camera_DCMI->double_buffer_first          = double_buffer_first;
            Camera_DCMI->double_buffer_second         = double_buffer_second;
            Camera_DCMI->half_middle_buffer_words     = half_middle_buffer_words;
            Camera_DCMI->double_buffer_transmit_count = double_buffer_transmit_count;
            Camera_DCMI->per_block_word               = per_block_word;
            Camera_DCMI->block_count                  = block_count;
            Camera_DCMI->final_buffer                 = final_buffer;
            Camera_DCMI->final_buffer_len             = final_buffer_len;
            Camera_DCMI->dma_not_full                 = dma_not_full;
            Camera_DCMI->jpeg_data_count_calculate    = 0;
            Camera_DCMI->the_node_list.resize(double_buffer_transmit_count - 1);
            debug("Double Buffer Transmit Count: %u\n", double_buffer_transmit_count);

            HAL_MDMA_DeInit(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_Init(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_REPBLOCKCPLT_CB_ID, camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_MDMA_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_MDMA_Error_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_MDMA_Abort_Cb);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            uintptr_t             destination = final_buffer + (half_middle_buffer_words << 2u);
            bool                  is_second   = true;
            MDMA_LinkNodeTypeDef *prev_node   = nullptr;
            for (auto &x : Camera_DCMI->the_node_list) {
                MDMA_LinkNodeConfTypeDef node_conf;
                node_conf.Init = Camera_DCMI->second_stage_dma.Init;
                if (&x == &(Camera_DCMI->the_node_list.back()) && dma_not_full) {
                    node_conf.Init.Request = MDMA_REQUEST_SW;
                }
                node_conf.BlockCount             = block_count;
                node_conf.BlockDataLength        = (per_block_word << 2u);
                node_conf.DstAddress             = destination;
                node_conf.SrcAddress             = is_second ? double_buffer_second : double_buffer_first;
                node_conf.PostRequestMaskAddress = uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR));
                node_conf.PostRequestMaskData    = 0b10000u;

                HAL_MDMA_LinkedList_CreateNode(&x, &node_conf);
                if (HAL_MDMA_LinkedList_AddNode(&Camera_DCMI->second_stage_dma, &x, prev_node) != HAL_OK) {
                    debug("Failed to add LinkList node\n");
                    return HAL_ERROR;
                }

                prev_node = &x;
                is_second = !is_second;
                destination += (half_middle_buffer_words << 2u);
            }
            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->the_node_list.data(),
                                                int32_t(Camera_DCMI->the_node_list.size() * sizeof(MDMA_LinkNodeConfTypeDef)));

            for (auto &x : Camera_DCMI->the_node_list) {
                debug("[Address: %x]\n", (unsigned)(&x));
                debug("CTCR:   %x\n", (unsigned)x.CTCR);
                debug("CBNDTR: %x\n", (unsigned)x.CBNDTR);
                debug("CSAR:   %x\n", (unsigned)x.CSAR);
                debug("CDAR:   %x\n", (unsigned)x.CDAR);
                debug("CBRUR:  %x\n", (unsigned)x.CBRUR);
                debug("CLAR:   %x\n", (unsigned)x.CLAR);
                debug("CTBR:   %x\n", (unsigned)x.CTBR);
                debug("CMAR:   %x\n", (unsigned)x.CMAR);
                debug("CMDR:   %x\n", (unsigned)x.CMDR);
                debug("\n");
            }

            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)final_buffer, (int32_t)final_buffer_len);
            Camera_DCMI->mdma_first_buffer = true;
            HAL_MDMA_Start_IT(&Camera_DCMI->second_stage_dma,
                              (uint32_t)double_buffer_first,
                              (uint32_t)final_buffer,
                              (uint32_t)(per_block_word << 2u),
                              (uint32_t)block_count);

            taskENTER_CRITICAL();
            {
                target_dcmi_is_ok = true;
            }
            taskEXIT_CRITICAL();

            // start dma and mdma
            HAL_DMA_DeInit(&(Camera_DCMI->first_stage_dma));
            HAL_DMA_Init(&(Camera_DCMI->first_stage_dma));
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_DMA_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_M1CPLT_CB_ID, camera_RGB_YCbCr_DMA_M1_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_DMA_Error_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_DMA_Abort_Cb);
            HAL_DMAEx_MultiBufferStart_IT(&(Camera_DCMI->first_stage_dma),
                                          (uint32_t)&(Camera_DCMI->instance.Instance->DR),
                                          (uint32_t)double_buffer_first,
                                          (uint32_t)double_buffer_second,
                                          (uint32_t)half_middle_buffer_words);
        }

        break;
    }
    case camera_format::format_JPEG: {
        configASSERT(middle_buffer % 4 == 0);
        configASSERT(middle_buffer_len % 32 == 0);
        configASSERT(final_buffer % 4 == 0);
        configASSERT(final_buffer_len % 32 == 0);

        const size_t mdma_single_block_max_words = 65536 / 4;

        size_t       middle_buffer_words         = middle_buffer_len >> 2u;
        size_t       half_middle_buffer_words    = (middle_buffer_words >> 1u) > 0xffff ? 0xffff : (middle_buffer_words >> 1u);
        size_t       per_block_word = 0, block_count = 0;

        Camera_DCMI->single_buffer_fine = false;
        Camera_DCMI->jpeg_mode          = true;

        calculate_decompose(half_middle_buffer_words, per_block_word, block_count, mdma_single_block_max_words);

        uintptr_t double_buffer_first             = middle_buffer;
        uintptr_t double_buffer_second            = middle_buffer + (half_middle_buffer_words << 2);
        size_t    double_buffer_transmit_count    = UINT32_MAX;

        Camera_DCMI->double_buffer_first          = double_buffer_first;
        Camera_DCMI->double_buffer_second         = double_buffer_second;
        Camera_DCMI->half_middle_buffer_words     = half_middle_buffer_words;
        Camera_DCMI->double_buffer_transmit_count = double_buffer_transmit_count;
        Camera_DCMI->per_block_word               = per_block_word;
        Camera_DCMI->block_count                  = block_count;
        Camera_DCMI->final_buffer                 = final_buffer;
        Camera_DCMI->final_buffer_len             = final_buffer_len;
        Camera_DCMI->dma_not_full                 = true;
        Camera_DCMI->jpeg_sw_final                = false;
        Camera_DCMI->jpeg_data_count_calculate    = 0;
        Camera_DCMI->the_node_list.resize(256);

        HAL_MDMA_DeInit(&Camera_DCMI->second_stage_dma);
        HAL_MDMA_Init(&Camera_DCMI->second_stage_dma);
        HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_REPBLOCKCPLT_CB_ID, camera_JPEG_MDMA_RepeatBlock_Cplt_Cb);
        HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_CPLT_CB_ID, camera_JPEG_MDMA_Cplt_Cb);
        HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ERROR_CB_ID, camera_JPEG_MDMA_Error_Cb);
        HAL_MDMA_RegisterCallback(&Camera_DCMI->second_stage_dma, HAL_MDMA_XFER_ABORT_CB_ID, camera_JPEG_MDMA_Abort_Cb);
        HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->second_stage_dma,
                                       uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR)),
                                       0b10000u);


        uintptr_t             destination = final_buffer + (half_middle_buffer_words << 2u);
        bool                  is_second   = true;
        MDMA_LinkNodeTypeDef *prev_node   = nullptr;
        for (auto &x : Camera_DCMI->the_node_list) {
            MDMA_LinkNodeConfTypeDef node_conf;
            node_conf.Init                   = Camera_DCMI->second_stage_dma.Init;
            node_conf.BlockCount             = block_count;
            node_conf.BlockDataLength        = (per_block_word << 2u);
            node_conf.DstAddress             = destination;
            node_conf.SrcAddress             = is_second ? double_buffer_second : double_buffer_first;
            node_conf.PostRequestMaskAddress = uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR));
            node_conf.PostRequestMaskData    = 0b10000u;

            HAL_MDMA_LinkedList_CreateNode(&x, &node_conf);
            if (HAL_MDMA_LinkedList_AddNode(&Camera_DCMI->second_stage_dma, &x, prev_node) != HAL_OK) {
                debug("Failed to add LinkList node\n");
                return HAL_ERROR;
            }

            prev_node = &x;
            is_second = !is_second;
            destination += (half_middle_buffer_words << 2u);
        }
        MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->the_node_list.data(),
                                            int32_t(Camera_DCMI->the_node_list.size() * sizeof(MDMA_LinkNodeConfTypeDef)));

        MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)final_buffer, (int32_t)final_buffer_len);

        Camera_DCMI->mdma_first_buffer = true;
        HAL_MDMA_Start_IT(&Camera_DCMI->second_stage_dma,
                          (uint32_t)double_buffer_first,
                          (uint32_t)final_buffer,
                          (uint32_t)(per_block_word << 2u),
                          (uint32_t)block_count);

        taskENTER_CRITICAL();
        {
            target_dcmi_is_ok = true;
        }
        taskEXIT_CRITICAL();

        // start dma and mdma
        HAL_DMA_DeInit(&(Camera_DCMI->first_stage_dma));
        HAL_DMA_Init(&(Camera_DCMI->first_stage_dma));
        HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_CPLT_CB_ID, camera_JPEG_DMA_Cplt_Cb);
        HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_M1CPLT_CB_ID, camera_JPEG_DMA_M1_Cplt_Cb);
        HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ERROR_CB_ID, camera_JPEG_DMA_Error_Cb);
        HAL_DMA_RegisterCallback(&(Camera_DCMI->first_stage_dma), HAL_DMA_XFER_ABORT_CB_ID, camera_JPEG_DMA_Abort_Cb);
        HAL_DMAEx_MultiBufferStart_IT(&(Camera_DCMI->first_stage_dma),
                                      (uint32_t)&(Camera_DCMI->instance.Instance->DR),
                                      (uint32_t)double_buffer_first,
                                      (uint32_t)double_buffer_second,
                                      (uint32_t)half_middle_buffer_words);

        break;
    }
    }

    Camera_DCMI->instance.State = HAL_DCMI_STATE_READY;

    __HAL_DCMI_ENABLE_IT(&Camera_DCMI->instance, DCMI_IT_FRAME);
    Camera_DCMI->instance.Instance->CR |= DCMI_CR_CAPTURE;
    __HAL_UNLOCK(&(Camera_DCMI->instance));

    return HAL_OK;
}

uint32_t camera_capture_process(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    EventBits_t message;
    uint32_t    ret = 0;
    message         = xEventGroupWaitBits(Camera_DCMI->eg, CAMERA_CAPTURE_ALL_MASK, pdTRUE, pdFALSE, 0);
    if (!message) {
        ret |= (uint32_t)message_process::CANNOT_GET;
    }

    if (message & FIRST_STAGE_DMA_ERROR) {
        ret |= (uint32_t)message_process::ERROR_STOP;
    }
    if (message & FIRST_STAGE_DMA_ABORT) {
        ret |= (uint32_t)message_process::DMA_ABORT;
    }
    if (message & FIRST_STAGE_DMA_CPLT) {
        ret |= (uint32_t)message_process::OK;
    }
    if (message & FIRST_STAGE_DMA_M1_CPLT) {
        ret |= (uint32_t)message_process::OK;
    }
    if (message & SECOND_STAGE_DMA_ERROR) {
        ret |= (uint32_t)message_process::ERROR_STOP;
    }
    if (message & SECOND_STAGE_DMA_ABORT) {
        ret |= (uint32_t)message_process::MDMA_ABORT;
    }
    if (message & SECOND_STAGE_DMA_CPLT) {
        ret |= (uint32_t)message_process::DATA_END;
    }
    if (message & SECOND_STAGE_DMA_REPEAT_CPLT) {
        ret |= (uint32_t)message_process::OK;
    }
    if (message & CAMERA_CAPTURE_OK) {
        ret |= (uint32_t)message_process::CAPTURE_END;
    }
    if (message & CAMERA_CAPTURE_ERROR) {
        ret |= (uint32_t)message_process::ERROR_STOP;
    }
    ret |= (uint32_t)message_process::CANNOT_GET;

    return ret;
}

HAL_StatusTypeDef camera_capture_resume(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    __HAL_LOCK(&Camera_DCMI->instance);

    Camera_DCMI->instance.State = HAL_DCMI_STATE_BUSY;

    __HAL_DCMI_ENABLE(&Camera_DCMI->instance);

    Camera_DCMI->instance.Instance->CR &= ~(DCMI_CR_CM);
    Camera_DCMI->instance.Instance->CR |= (uint32_t)(DCMI_MODE_SNAPSHOT);

    switch (target_format) {
    case camera_format::format_RGB:
    case camera_format::format_YCbCr: {

        if (Camera_DCMI->single_buffer_fine) { // single buffer is fine
            HAL_MDMA_Init(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            Camera_DCMI->mdma_first_buffer = true;
            HAL_MDMA_Start_IT(&Camera_DCMI->second_stage_dma,
                              (uint32_t)(Camera_DCMI->double_buffer_first),
                              (uint32_t)(Camera_DCMI->final_buffer),
                              (uint32_t)(Camera_DCMI->per_block_word << 2u),
                              (uint32_t)(Camera_DCMI->block_count));

            taskENTER_CRITICAL();
            {
                target_dcmi_is_ok = true;
            }
            taskEXIT_CRITICAL();

            // start dma and mdma
            HAL_DMA_Init(&(Camera_DCMI->first_stage_dma));
            HAL_DMA_Start_IT(&(Camera_DCMI->first_stage_dma),
                             (uint32_t)&(Camera_DCMI->instance.Instance->DR),
                             (uint32_t)(Camera_DCMI->double_buffer_first),
                             (uint32_t)(Camera_DCMI->half_middle_buffer_words));
        }
        else { // double buffer!
            HAL_MDMA_Init(&Camera_DCMI->second_stage_dma);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            MDMA_LinkNodeTypeDef *prev_node = nullptr;
            for (auto &x : Camera_DCMI->the_node_list) {
                if (HAL_MDMA_LinkedList_AddNode(&Camera_DCMI->second_stage_dma, &x, prev_node) != HAL_OK) {
                    debug("Failed to add LinkList node\n");
                    return HAL_ERROR;
                }
                prev_node = &x;
            }

            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->the_node_list.data(),
                                                int32_t(Camera_DCMI->the_node_list.size() * sizeof(MDMA_LinkNodeConfTypeDef)));
            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->final_buffer, (int32_t)Camera_DCMI->final_buffer_len);
            Camera_DCMI->mdma_first_buffer = true;
            HAL_MDMA_Start_IT(&Camera_DCMI->second_stage_dma,
                              (uint32_t)Camera_DCMI->double_buffer_first,
                              (uint32_t)Camera_DCMI->final_buffer,
                              (uint32_t)(Camera_DCMI->per_block_word << 2u),
                              (uint32_t)Camera_DCMI->block_count);

            taskENTER_CRITICAL();
            {
                target_dcmi_is_ok = true;
            }
            taskEXIT_CRITICAL();

            // start dma and mdma
            HAL_DMA_Init(&(Camera_DCMI->first_stage_dma));
            HAL_DMAEx_MultiBufferStart_IT(&(Camera_DCMI->first_stage_dma),
                                          (uint32_t)&(Camera_DCMI->instance.Instance->DR),
                                          (uint32_t)Camera_DCMI->double_buffer_first,
                                          (uint32_t)Camera_DCMI->double_buffer_second,
                                          (uint32_t)Camera_DCMI->half_middle_buffer_words);
        }

        break;
    }
    case camera_format::format_JPEG: {
        break;
    }
    }

    Camera_DCMI->instance.State = HAL_DCMI_STATE_READY;

    __HAL_DCMI_ENABLE_IT(&Camera_DCMI->instance, DCMI_IT_FRAME);
    Camera_DCMI->instance.Instance->CR |= DCMI_CR_CAPTURE;
    __HAL_UNLOCK(&(Camera_DCMI->instance));

    return HAL_OK;
}

void camera_RGB_YCbCr_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    HAL_DMA_Abort_IT(&Camera_DCMI->first_stage_dma);

    while (Camera_DCMI->instance.Instance->SR & DCMI_SR_FNE_Msk) {
        vTaskDelay(1); // yield 1 time slice
    }

    if (Camera_DCMI->dma_not_full) {
        xQueueSend(Camera_DCMI->MDMA_sync, nullptr, portMAX_DELAY);
        HAL_MDMA_GenerateSWRequest(&Camera_DCMI->second_stage_dma);
    }
    __HAL_DCMI_DISABLE(&Camera_DCMI->instance);
}

void camera_RGB_YCbCr_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    __HAL_DCMI_DISABLE_IT(&Camera_DCMI->instance, DCMI_IT_FRAME);
    Camera_DCMI->instance.Instance->CR &= ~DCMI_CR_CAPTURE;

    while (Camera_DCMI->instance.Instance->CR & DCMI_CR_CAPTURE) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    __HAL_DCMI_DISABLE(&Camera_DCMI->instance);

    HAL_DMA_Abort(&Camera_DCMI->first_stage_dma);
    HAL_MDMA_Abort(&Camera_DCMI->second_stage_dma);
    xEventGroupClearBits(Camera_DCMI->eg, 0x00ffffff);
    xQueueReset(camera_new_message);

    __HAL_DCMI_CLEAR_FLAG(&Camera_DCMI->instance, 0x1F);
    __HAL_DMA_CLEAR_FLAG(&Camera_DCMI->first_stage_dma, 0x3D);
    __HAL_MDMA_CLEAR_FLAG(&Camera_DCMI->second_stage_dma, 0x1F);

    taskENTER_CRITICAL();
    {
        target_dcmi_is_ok = false;
    }
    taskEXIT_CRITICAL();
}


void camera_JPEG_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    while (Camera_DCMI->instance.Instance->SR & DCMI_SR_FNE_Msk) {
        vTaskDelay(1); // yield 1 time slice
    }

    uint32_t remain_words    = ((DMA_Stream_TypeDef *)(Camera_DCMI->first_stage_dma.Instance))->NDTR;
    bool     need_sw_trigger = (remain_words != Camera_DCMI->half_middle_buffer_words);

    Camera_DCMI->jpeg_data_count_calculate += (Camera_DCMI->half_middle_buffer_words - remain_words);
    HAL_DMA_Abort_IT(&Camera_DCMI->first_stage_dma);

    if (need_sw_trigger) {
        // Just a trick!
        xQueueSend(Camera_DCMI->MDMA_sync, nullptr, portMAX_DELAY);
        if (Camera_DCMI->second_stage_dma.Instance->CCR & MDMA_CCR_EN) {
            Camera_DCMI->second_stage_dma.Instance->CCR &= (~MDMA_CCR_EN);
            Camera_DCMI->second_stage_dma.Instance->CTCR |= (MDMA_CTCR_SWRM);
            Camera_DCMI->second_stage_dma.Instance->CCR |= (MDMA_CCR_EN);
            Camera_DCMI->jpeg_sw_final = true;
            debug("JPEG SW Request!\n");
            if (HAL_MDMA_GenerateSWRequest(&Camera_DCMI->second_stage_dma) != HAL_OK) {
                debug("SWR FAILED!\n");
            }
        }
        else {
            debug("[BullShit Error] " __FILE__ ":%d\n", __LINE__);
        }
    }
    else {
        xEventGroupSetBits(Camera_DCMI->eg, SECOND_STAGE_DMA_CPLT);
        xSemaphoreGive(camera_new_message);
    }

    __HAL_DCMI_DISABLE(&Camera_DCMI->instance);
}

void camera_JPEG_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    __HAL_DCMI_DISABLE_IT(&Camera_DCMI->instance, DCMI_IT_FRAME);
    Camera_DCMI->instance.Instance->CR &= ~DCMI_CR_CAPTURE;

    while (Camera_DCMI->instance.Instance->CR & DCMI_CR_CAPTURE) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    __HAL_DCMI_DISABLE(&Camera_DCMI->instance);

    HAL_DMA_Abort(&Camera_DCMI->first_stage_dma);
    HAL_MDMA_Abort(&Camera_DCMI->second_stage_dma);
    xEventGroupClearBits(Camera_DCMI->eg, 0x00ffffff);
    xQueueReset(camera_new_message);

    __HAL_DCMI_CLEAR_FLAG(&Camera_DCMI->instance, 0x1F);
    __HAL_DMA_CLEAR_FLAG(&Camera_DCMI->first_stage_dma, 0x3D);
    __HAL_MDMA_CLEAR_FLAG(&Camera_DCMI->second_stage_dma, 0x1F);

    taskENTER_CRITICAL();
    {
        target_dcmi_is_ok = false;
    }
    taskEXIT_CRITICAL();
}

void calculate_decompose(size_t &x, size_t &y, size_t &z, size_t y_max) {
    y = x;
    z = 1;
    //
    size_t t1 = 0, t2 = 0;

    while (y > y_max) {
        if (y % 2) {
            t2 += 1u << t1;
        }
        y >>= 1u;
        z <<= 1u;
        t1++;
    }

    x -= t2;
}

void resolution_parse(uint32_t &resolution, uint32_t &data_length, uint32_t &src_w, uint32_t &src_h, uint32_t &format) {
    switch (current_resolution) {
    case camera_resolution::reso_5M: {
        resolution  = OV5640_R2592x1944;
        data_length = 2592 * 1944;
        src_w       = 2592;
        src_h       = 1944;
        break;
    }
    case camera_resolution::reso_QXGA: {
        resolution  = OV5640_R2048x1536;
        data_length = 2048 * 1536;
        src_w       = 2048;
        src_h       = 1536;
        break;
    }
    case camera_resolution::reso_1080p: {
        resolution  = OV5640_R1920x1080;
        data_length = 1920 * 1080;
        src_w       = 1920;
        src_h       = 1080;
        break;
    }
    case camera_resolution::reso_UXGA: {
        resolution  = OV5640_R1600x1200;
        data_length = 1600 * 1200;
        src_w       = 1600;
        src_h       = 1200;
        break;
    }
    case camera_resolution::reso_SXGA: {
        resolution  = OV5640_R1280x1024;
        data_length = 1280 * 1024;
        src_w       = 1280;
        src_h       = 1024;
        break;
    }
    case camera_resolution::reso_WXGA_plus: {
        resolution  = OV5640_R1440x900;
        data_length = 1440 * 900;
        src_w       = 1440;
        src_h       = 900;
        break;
    }
    case camera_resolution::reso_WXGA: {
        resolution  = OV5640_R1280x800;
        data_length = 1280 * 800;
        src_w       = 1280;
        src_h       = 800;
        break;
    }
    case camera_resolution::reso_XGA: {
        resolution  = OV5640_R1024x768;
        data_length = 1024 * 768;
        src_w       = 1024;
        src_h       = 768;
        break;
    }
    case camera_resolution::reso_SVGA: {
        resolution  = OV5640_R800x600;
        data_length = 800 * 600;
        src_w       = 800;
        src_h       = 600;
        break;
    }
    case camera_resolution::reso_WVGA: {
        resolution  = OV5640_R800x480;
        data_length = 800 * 480;
        src_w       = 800;
        src_h       = 480;
        break;
    }
    case camera_resolution::reso_VGA: {
        resolution  = OV5640_R640x480;
        data_length = 640 * 480;
        src_w       = 640;
        src_h       = 480;
        break;
    }
    case camera_resolution::reso_PSP: {
        resolution  = OV5640_R480x272;
        data_length = 480 * 272;
        src_w       = 480;
        src_h       = 272;
        break;
    }
    case camera_resolution::reso_QVGA: {
        resolution  = OV5640_R320x240;
        data_length = 320 * 240;
        src_w       = 320;
        src_h       = 240;
        break;
    }
    case camera_resolution::reso_QQVGA: {
        resolution  = OV5640_R160x120;
        data_length = 160 * 120;
        src_w       = 160;
        src_h       = 120;
        break;
    }
    }


    switch (current_format) {
    case camera_format::format_RGB: {
        format = OV5640_RGB565;
        data_length *= 2;
        target_dcmi = &(RGB_hdcmi);
        break;
    }
    case camera_format::format_YCbCr: {
        format = OV5640_YUV422;
        data_length *= 2;
        target_dcmi = &(YCbCr_hdcmi);
        break;
    }
    case camera_format::format_JPEG: {
        format      = OV5640_JPEG;
        data_length = 0;
        target_dcmi = &(JPEG_hdcmi);
        break;
    }
    }
}

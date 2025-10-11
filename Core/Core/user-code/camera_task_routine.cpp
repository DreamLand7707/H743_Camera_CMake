#define FILE_DEBUG 1

#include "camera_declare.hpp"

#define ALINTEK_BOARD

static void lvgl_create_camera_interface();
static void dcmi_resource_init();

static void change_to_file_explorer_callback(lv_event_t *e);
static void take_photo_callback(lv_event_t *e);
static void open_setting_callback(lv_event_t *e);
//
static void         indicator_operate(const char *message);
static void         screen_image_operate(void *source);

extern "C" void     DMA1_Stream0_IRQHandler(void);


DCMI_HandleTypeDef  RGB_hdcmi;   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
DCMI_HandleTypeDef  YCbCr_hdcmi; // YCbCr(4:2:2) => Encode Inside(JPEG)
DCMI_HandleTypeDef  JPEG_hdcmi;  // JPEG(4:2:2)
DCMI_HandleTypeDef *target_dcmi;
DMA_HandleTypeDef   my_hdma_dcmi;

OV5640_IO_t         ov5640_io {};
OV5640_Object_t     ov5640 {};
soft_sccb_handle    my_sccb;

camera_resolution   current_resolution;
camera_format       current_format;

SemaphoreHandle_t   camera_interface_changed;
bool                PCF8574_init = false;

namespace
{
    SemaphoreHandle_t camera_new_scene;
    SemaphoreHandle_t camera_exit;
    QueueSetHandle_t  camera_queue_set;
} // namespace

namespace
{
    lv_obj_t *screen_container;

    lv_obj_t *camera_capture_image;
    lv_obj_t *button_container;

    lv_obj_t *take_photo_button;
    lv_obj_t *change_to_file_explorer_button;
    lv_obj_t *open_setting_button;
    lv_obj_t *take_photo_label;
    lv_obj_t *change_to_file_explorer_label;
    lv_obj_t *open_setting_label;

    lv_obj_t *indicator_label;
} // namespace

void camera_task_routine(void const *argument) {
    static lv_image_dsc_t img_dsc;
    img_dsc.header.magic      = LV_IMAGE_HEADER_MAGIC;
    img_dsc.header.cf         = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.flags      = 0;
    img_dsc.header.reserved_2 = 0;

    xSemaphoreTake(sema_camera_routine_start, portMAX_DELAY);

    dcmi_data_structure_init();
    dcmi_io_deinit_ov5640();

    xSemaphoreGive(sema_camera_ov5640_unable_done);

    lvgl_create_camera_interface();
    dcmi_resource_init();

    xSemaphoreGive(sema_camera_routine_init_done);


    while (1) {
        xSemaphoreTake(camera_interface_changed, portMAX_DELAY);

        uint32_t      resolution, format, data_length, src_w, src_h;
        bool          can_catch_scene;
        QueueHandle_t the_queue;

        switch (current_resolution) {
        case camera_resolution::reso_max_resolution: {
            resolution  = 0;
            data_length = 2592 * 1944;
            src_w       = 2592;
            src_h       = 1944;
            break;
        }
        case camera_resolution::reso_1080p: {
            resolution  = 1;
            data_length = 1920 * 1080;
            src_w       = 1920;
            src_h       = 1080;
            break;
        }
        case camera_resolution::reso_vga: {
            resolution  = OV5640_R640x480;
            data_length = 640 * 480;
            src_w       = 640;
            src_h       = 480;
            break;
        }
        }


        switch (current_format) {
        case camera_format::format_RGB: {
            format = OV5640_RGB565;
            data_length *= 2;
            can_catch_scene = true;
            target_dcmi     = &RGB_hdcmi;
            break;
        }
        case camera_format::format_YCbCr: {
            format = OV5640_YUV422;
            data_length *= 2;
            can_catch_scene = false;
            target_dcmi     = &YCbCr_hdcmi;
            break;
        }
        case camera_format::format_JPEG: {
            format          = OV5640_JPEG;
            data_length     = 0;
            can_catch_scene = false;
            target_dcmi     = &JPEG_hdcmi;
            break;
        }
        }

        // DCMI Init -> IO Init -> OV5640 Init -> OV5640 Start -> DCMI DMA
        indicator_operate("Initialize DCMI Interface...");
        if (HAL_DCMI_Init(target_dcmi) != HAL_OK) {
            indicator_operate("Initialize DCMI Interface Failed!");
            continue;
        }

        indicator_operate("Initialize Camera...");
        (void)OV5640_RegisterBusIO(&ov5640, &ov5640_io);
        if (OV5640_Init(&ov5640, resolution, format) != OV5640_OK) {
            indicator_operate("Initialize Camera Failed!");
            continue;
        }

        indicator_operate("Start Camera...");
        OV5640_Start(&ov5640);

        indicator_operate(nullptr);

        if (can_catch_scene) {
            HAL_DCMI_Start_DMA(target_dcmi, DCMI_MODE_SNAPSHOT, (uint32_t)jpeg_before_buffer_rgb, data_length / 4);
        }

        while (1) {
            the_queue = xQueueSelectFromSet(camera_queue_set, portMAX_DELAY);
            if (the_queue == camera_new_scene && can_catch_scene) {
                xSemaphoreTake(camera_new_scene, 0);

                if (HAL_DCMI_Stop(target_dcmi) != HAL_OK) {
                    debug("Can't Stop DCMI!\n");
                }
                else {
                    debug("Stop DCMI!\n");
                }

                full_pict_show_size[0] = MY_DISP_HOR_RES;
                full_pict_show_size[1] = MY_DISP_VER_RES;
                picture_scaling(jpeg_before_buffer_rgb, full_screen_pict_show_buffer,
                                src_w, src_h,
                                full_pict_show_size[0], full_pict_show_size[1]);

                img_dsc.header.w  = full_pict_show_size[0];
                img_dsc.header.h  = full_pict_show_size[1];
                img_dsc.data      = full_screen_pict_show_buffer;
                img_dsc.data_size = full_pict_show_size[0] * full_pict_show_size[1] * 2;

                screen_image_operate(&img_dsc);

                __HAL_DCMI_ENABLE_IT(target_dcmi, DCMI_IT_FRAME);
                if (HAL_DCMI_Start_DMA(target_dcmi, DCMI_MODE_SNAPSHOT, (uint32_t)jpeg_before_buffer_rgb, data_length / 4) != HAL_OK) {
                    debug("Can't Start DCMI!\n");
                }
                else {
                    debug("Start DCMI!\n");
                }
            }
            else if (the_queue == camera_exit) {
                xSemaphoreTake(camera_exit, 0);

                indicator_operate(nullptr);
                screen_image_operate(nullptr);

                // Init:   DCMI Init   -> IO Init   -> OV5640 Init   -> OV5640 Start -> DCMI DMA
                // DeInit: DCMI DeInit <- IO DeInit <- OV5640 Deinit <- OV5640 Stop  <- DCMI DMA Stop
                HAL_DCMI_Stop(target_dcmi);
                OV5640_Stop(&ov5640);
                OV5640_DeInit(&ov5640);
                dcmi_io_deinit_ov5640();
                HAL_DCMI_DeInit(target_dcmi);

                vTaskDelay(pdMS_TO_TICKS(20));

                send_command_to_main_manage(nullptr, 0, manage_command_type::to_file_explorer, portMAX_DELAY);
                break;
            }
        }
    }
}


static void lvgl_create_camera_interface() {
    camera_screen    = lv_obj_create(nullptr);

    screen_container = lv_obj_create(camera_screen);
    lv_obj_set_style_size(screen_container, LV_PCT(100), LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(screen_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(screen_container, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(screen_container, 0, LV_STATE_DEFAULT);

    camera_capture_image = lv_image_create(camera_screen);
    lv_obj_set_style_size(camera_capture_image, LV_PCT(100), LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(camera_capture_image, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(camera_capture_image, 0, LV_STATE_DEFAULT);

    button_container = lv_obj_create(camera_screen);
    lv_obj_set_style_size(button_container, LV_PCT(15), LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(button_container, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(button_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(button_container, lv_color_hex(0x555555), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(button_container, 64, LV_STATE_DEFAULT);
    lv_obj_set_style_align(button_container, LV_ALIGN_RIGHT_MID, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_gap(button_container, 40, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(button_container, 0, LV_STATE_DEFAULT);

    lv_obj_set_layout(button_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_COLUMN);

    take_photo_button              = lv_button_create(button_container);
    open_setting_button            = lv_button_create(button_container);
    change_to_file_explorer_button = lv_button_create(button_container);

    lv_obj_move_to_index(open_setting_button, 0);
    lv_obj_move_to_index(take_photo_button, 1);
    lv_obj_move_to_index(change_to_file_explorer_button, 2);

    lv_obj_set_style_bg_color(take_photo_button, lv_color_hex(0x5ac3dd), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(open_setting_button, lv_color_hex(0x5ac3dd), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(change_to_file_explorer_button, lv_color_hex(0x5ac3dd), LV_STATE_DEFAULT);

    lv_obj_set_style_width(open_setting_button, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_width(take_photo_button, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_width(change_to_file_explorer_button, LV_PCT(100), LV_STATE_DEFAULT);

    lv_obj_set_style_flex_grow(open_setting_button, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_flex_grow(take_photo_button, 1, LV_STATE_DEFAULT);
    lv_obj_set_style_flex_grow(change_to_file_explorer_button, 1, LV_STATE_DEFAULT);


    take_photo_label              = lv_label_create(take_photo_button);
    change_to_file_explorer_label = lv_label_create(change_to_file_explorer_button);
    open_setting_label            = lv_label_create(open_setting_button);

    lv_obj_set_style_text_font(take_photo_label, &photo_folder_setting, LV_STATE_DEFAULT);

    lv_label_set_text_static(take_photo_label, MY_TAKE_PHOTO_SYMBOL);
    lv_label_set_text_static(change_to_file_explorer_label, LV_SYMBOL_FILE);
    lv_label_set_text_static(open_setting_label, LV_SYMBOL_HOME);

    lv_obj_set_align(change_to_file_explorer_label, LV_ALIGN_CENTER);
    lv_obj_set_align(take_photo_label, LV_ALIGN_CENTER);
    lv_obj_set_align(open_setting_label, LV_ALIGN_CENTER);

    lv_obj_add_event_cb(change_to_file_explorer_button, change_to_file_explorer_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(take_photo_button, take_photo_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(open_setting_button, open_setting_callback, LV_EVENT_CLICKED, nullptr);

    indicator_label = lv_label_create(camera_screen);
    lv_obj_set_align(indicator_label, LV_ALIGN_CENTER);
    lv_label_set_text_static(indicator_label, "");
    lv_obj_add_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(indicator_label, lv_color_hex(0xdddddd), LV_STATE_DEFAULT);
}

static void dcmi_resource_init() {
    camera_interface_changed = xSemaphoreCreateBinary();
    camera_queue_set         = xQueueCreateSet(2);
    camera_new_scene         = xSemaphoreCreateBinary();
    camera_exit              = xSemaphoreCreateBinary();
    xQueueAddToSet(camera_new_scene, camera_queue_set);
    xQueueAddToSet(camera_exit, camera_queue_set);
}

static void change_to_file_explorer_callback(lv_event_t *e) {
    xSemaphoreGive(camera_exit);
}

static void take_photo_callback(lv_event_t *e) {}
static void open_setting_callback(lv_event_t *e) {}


//


static HAL_StatusTypeDef hdmi_start_capture(DCMI_HandleTypeDef *hdcmi, camera_format target_format,
                                            uint8_t *dma_first_buffer, size_t dma_first_buffer_length,
                                            uint8_t *final_data, size_t data_length) {
    __HAL_LOCK(hdcmi);

    hdcmi->State = HAL_DCMI_STATE_BUSY;

    __HAL_DCMI_ENABLE(hdcmi);

    hdcmi->Instance->CR &= ~(DCMI_CR_CM);
    hdcmi->Instance->CR |= (uint32_t)(DCMI_MODE_SNAPSHOT);

    switch (target_format) {
    case camera_format::format_RGB:
    case camera_format::format_YCbCr: {

        size_t final_data_item_number     = data_length >> 4;
        size_t dma_buffer_max_item_number = std::max<size_t>(dma_first_buffer_length >> 4, 0xffff);

        bool   need_double_buffer         = final_data_item_number > dma_buffer_max_item_number;


        if (need_double_buffer) {
            size_t times     = final_data_item_number / dma_buffer_max_item_number;
            size_t remainder = final_data_item_number % dma_buffer_max_item_number;

            if (remainder) {
                if (times % 2)
                    times++;
                else
                    times += 2;
            }
        }
        else {
        }

        break;
    }
    case camera_format::format_JPEG: {
        break;
    }
    }

    /* Set the DMA memory0 conversion complete callback */
    // hdcmi->DMA_Handle->XferCpltCallback = DCMI_DMAXferCplt;

    /* Set the DMA error callback */
    // hdcmi->DMA_Handle->XferErrorCallback = DCMI_DMAError;

    /* Set the dma abort callback */
    // hdcmi->DMA_Handle->XferAbortCallback = NULL;

    /* Reset transfer counters value */
    // hdcmi->XferCount          = 0;
    // hdcmi->XferTransferNumber = 0;
    // hdcmi->XferSize           = 0;
    // hdcmi->pBuffPtr           = 0;

    // if (Length <= 0xFFFFU) {
    //     /* Enable the DMA Stream */
    //     if (HAL_DMA_Start_IT(hdcmi->DMA_Handle, (uint32_t)&hdcmi->Instance->DR, (uint32_t)pData, Length) != HAL_OK) {
    //         /* Set Error Code */
    //         hdcmi->ErrorCode = HAL_DCMI_ERROR_DMA;
    //         /* Change DCMI state */
    //         hdcmi->State = HAL_DCMI_STATE_READY;
    //         /* Release Lock */
    //         __HAL_UNLOCK(hdcmi);
    //         /* Return function status */
    //         return HAL_ERROR;
    //     }
    // }
    // else /* DCMI_DOUBLE_BUFFER Mode */
    // {
    /* Set the DMA memory1 conversion complete callback */
    // hdcmi->DMA_Handle->XferM1CpltCallback = DCMI_DMAXferCplt;

    /* Initialize transfer parameters */
    // hdcmi->XferCount = 1;
    // hdcmi->XferSize  = Length;
    // hdcmi->pBuffPtr  = pData;

    // /* Get the number of buffer */
    // while (hdcmi->XferSize > 0xFFFFU) {
    //     hdcmi->XferSize  = (hdcmi->XferSize / 2U);
    //     hdcmi->XferCount = hdcmi->XferCount * 2U;
    // }

    // /* Update DCMI counter  and transfer number*/
    // hdcmi->XferCount          = (hdcmi->XferCount - 2U);
    // hdcmi->XferTransferNumber = hdcmi->XferCount;

    // /* Update second memory address */
    // SecondMemAddress = (uint32_t)(pData + (4U * hdcmi->XferSize));

    // /* Start DMA multi buffer transfer */
    // if (HAL_DMAEx_MultiBufferStart_IT(hdcmi->DMA_Handle, (uint32_t)&hdcmi->Instance->DR, (uint32_t)pData, SecondMemAddress, hdcmi->XferSize) != HAL_OK) {
    //     /* Set Error Code */
    //     hdcmi->ErrorCode = HAL_DCMI_ERROR_DMA;
    //     /* Change DCMI state */
    //     hdcmi->State = HAL_DCMI_STATE_READY;
    //     /* Release Lock */
    //     __HAL_UNLOCK(hdcmi);
    //     /* Return function status */
    //     return HAL_ERROR;
    // }
    // }

    /* Enable Capture */
    hdcmi->Instance->CR |= DCMI_CR_CAPTURE;

    /* Release Lock */
    __HAL_UNLOCK(hdcmi);

    /* Return function status */
    return HAL_OK;
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    BaseType_t should_yield = pdFALSE;
    xSemaphoreGiveFromISR(camera_new_scene, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi) {
}

static void indicator_operate(const char *message) {
    if (!message) {
        lv_lock();
        {
            lv_obj_add_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, "");
        }
        lv_unlock();
    }
    else {
        lv_lock();
        {
            lv_obj_remove_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, message);
        }
        lv_unlock();
    }
}

static void screen_image_operate(void *source) {
    lv_lock();
    {
        lv_image_set_src(camera_capture_image, source);
        if (source) {
            lv_image_set_align(camera_capture_image, LV_IMAGE_ALIGN_CENTER);
        }
    }
    lv_unlock();
}
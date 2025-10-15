#define FILE_DEBUG 1

#include "camera_declare.hpp"

static void lvgl_create_camera_interface();
static void dcmi_resource_init();
static void camera_deinit();

static void camera_RGB_YCbCr_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma);
static void camera_RGB_YCbCr_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma);
static void camera_RGB_YCbCr_DMA_Error_Cb(DMA_HandleTypeDef *hdma);
static void camera_RGB_YCbCr_DMA_Abort_Cb(DMA_HandleTypeDef *hdma);
static void camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
static void camera_RGB_YCbCr_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
static void camera_RGB_YCbCr_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma);
static void camera_RGB_YCbCr_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma);

static void change_to_file_explorer_callback(lv_event_t *e);
static void take_photo_callback(lv_event_t *e);
static void open_setting_callback(lv_event_t *e);

// /
static void indicator_operate(const char *message);
static void screen_image_operate(void *source);
static void calculate_decompose(size_t &x, size_t &y, size_t &z, size_t y_max);
//
static uint32_t          camera_RGB_YCbCr_capture_process(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
static HAL_StatusTypeDef camera_start_capture(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format,
                                              uintptr_t middle_buffer, size_t middle_buffer_len,
                                              uintptr_t final_buffer, size_t final_buffer_len);
static HAL_StatusTypeDef camera_RGB_YCbCr_capture_resume(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
static void              camera_RGB_YCbCr_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
static void              camera_RGB_YCbCr_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
//

static char       error_message[128] {};

extern "C" void   DMA1_Stream0_IRQHandler(void);


OV5640_IO_t       ov5640_io {};
OV5640_Object_t   ov5640 {};
soft_sccb_handle  my_sccb {};

camera_resolution current_resolution {};
camera_format     current_format {};

SemaphoreHandle_t camera_interface_changed {};
bool              PCF8574_init            = false;
bool              camera_deinit_have_done = true;

namespace
{
    SemaphoreHandle_t camera_new_message {};
    SemaphoreHandle_t camera_exit {};
    SemaphoreHandle_t camera_error {};
    QueueSetHandle_t  camera_queue_set {};
} // namespace

namespace
{
    lv_obj_t *screen_container {};

    lv_obj_t *camera_capture_image {};
    lv_obj_t *button_container {};

    lv_obj_t *take_photo_button {};
    lv_obj_t *change_to_file_explorer_button {};
    lv_obj_t *open_setting_button {};
    lv_obj_t *take_photo_label {};
    lv_obj_t *change_to_file_explorer_label {};
    lv_obj_t *open_setting_label {};

    lv_obj_t *indicator_label {};
} // namespace


Camera_DCMI_HandleType  RGB_hdcmi {};   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
Camera_DCMI_HandleType  YCbCr_hdcmi {}; // YCbCr(4:2:2) => Encode Inside(JPEG)
Camera_DCMI_HandleType  JPEG_hdcmi {};  // JPEG(4:2:2)
Camera_DCMI_HandleType *target_dcmi {};
bool                    target_dcmi_is_ok = false;

void                    camera_task_routine(void const *argument) {
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

    bool          have_checked        = false;
    bool          can_catch_scene     = false;
    bool          camera_captured_end = false;
    QueueHandle_t the_queue {};
    uint32_t      resolution {}, format {}, data_length {}, src_w {}, src_h {};
    while (1) {
        // ?
        the_queue = xQueueSelectFromSet(camera_queue_set, portMAX_DELAY);

        if (the_queue == camera_interface_changed) {
            xSemaphoreTake(camera_interface_changed, 0);
            have_checked            = true;
            camera_deinit_have_done = false;
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
                target_dcmi     = &(RGB_hdcmi);
                break;
            }
            case camera_format::format_YCbCr: {
                format = OV5640_YUV422;
                data_length *= 2;
                can_catch_scene = false;
                target_dcmi     = &(YCbCr_hdcmi);
                break;
            }
            case camera_format::format_JPEG: {
                format          = OV5640_JPEG;
                data_length     = 0;
                can_catch_scene = false;
                target_dcmi     = &(JPEG_hdcmi);
                break;
            }
            }

            // DCMI Init -> IO Init -> OV5640 Init -> OV5640 Start -> DCMI DMA
            indicator_operate("Initialize DCMI Interface...");
            if (HAL_DCMI_Init(&(target_dcmi->data.instance)) != HAL_OK) {
                indicator_operate("Initialize DCMI Interface Failed!");
                can_catch_scene = false;
                continue;
            }

            indicator_operate("Initialize Camera...");
            (void)OV5640_RegisterBusIO(&ov5640, &ov5640_io);
            if (OV5640_Init(&ov5640, resolution, format) != OV5640_OK) {
                indicator_operate("Initialize Camera Failed!");
                can_catch_scene = false;
                continue;
            }

            indicator_operate("Start Camera...");
            OV5640_Start(&ov5640);

            indicator_operate(nullptr);

            if (can_catch_scene) {
                camera_captured_end = false;
                camera_start_capture(target_dcmi, current_format, 0x30000000, 128 * 1024, (uintptr_t)jpeg_before_buffer_rgb, data_length);
            }
        }
        else if (the_queue == camera_new_message && can_catch_scene) {
            xSemaphoreTake(camera_new_message, 0);
            if (!have_checked)
                continue;

            auto ret = camera_RGB_YCbCr_capture_process(target_dcmi, current_format);

            if ((ret & (uint32_t)message_process::ERROR_STOP)) {
                camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
                camera_captured_end = false;
                continue;
            }

            if (ret & (uint32_t)message_process::CAPTURE_END) {
                camera_captured_end = true;
                camera_RGB_YCbCr_capture_abort_first_stage_dma(target_dcmi, current_format);
            }

            if (ret & (uint32_t)message_process::DATA_END || ret & (uint32_t)message_process::MDMA_ABORT) {
                if (!camera_captured_end) {
                    debug("It seems not right\n");
                }
                camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
                camera_captured_end = false;

                MYSCB_InvalidateDCache_by_Addr((void *)jpeg_before_buffer_rgb, (int32_t)data_length);
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
                camera_RGB_YCbCr_capture_resume(target_dcmi, current_format);
            }
        }
        else if (the_queue == camera_exit) {
            xSemaphoreTake(camera_exit, 0);
            if (!have_checked)
                continue;
            have_checked = false;

            indicator_operate(nullptr);
            screen_image_operate(nullptr);

            // Init:   DCMI Init   -> IO Init   -> OV5640 Init   -> OV5640 Start -> DCMI DMA
            // DeInit: DCMI DeInit <- IO DeInit <- OV5640 Deinit <- OV5640 Stop  <- DCMI DMA Stop

            camera_deinit();
            vTaskDelay(pdMS_TO_TICKS(20));

            send_command_to_main_manage(nullptr, 0, manage_command_type::to_file_explorer, portMAX_DELAY);
        }
        else if (the_queue == camera_error) {
            xSemaphoreTake(camera_error, 0);

            indicator_operate(error_message);
            screen_image_operate(nullptr);

            camera_deinit();

            can_catch_scene = false;
        }
    }
}

static void camera_deinit() {
    if (!camera_deinit_have_done) {
        camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
        OV5640_Stop(&ov5640);
        OV5640_DeInit(&ov5640);
        dcmi_io_deinit_ov5640();
        HAL_DCMI_DeInit(&(target_dcmi->data.instance));
        camera_deinit_have_done = true;
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
    camera_queue_set         = xQueueCreateSet(4);
    camera_interface_changed = xSemaphoreCreateBinary();
    camera_new_message       = xSemaphoreCreateBinary();
    camera_exit              = xSemaphoreCreateBinary();
    camera_error             = xSemaphoreCreateBinary();
    xQueueAddToSet(camera_new_message, camera_queue_set);
    xQueueAddToSet(camera_exit, camera_queue_set);
    xQueueAddToSet(camera_error, camera_queue_set);
    xQueueAddToSet(camera_interface_changed, camera_queue_set);
}

static void change_to_file_explorer_callback(lv_event_t *e) {
    xSemaphoreGive(camera_exit);
}

static void take_photo_callback(lv_event_t *e) {}
static void open_setting_callback(lv_event_t *e) {}


//

// middle_buffer & final_data MUST 4 Byte Align
// middle_buffer_len MUST 32 Byte Align
// final_data_length MUST 32 Byte Align
static HAL_StatusTypeDef camera_start_capture(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format,
                                              uintptr_t middle_buffer, size_t middle_buffer_len,
                                              uintptr_t final_buffer, size_t final_buffer_len) {
    __HAL_LOCK(&Camera_DCMI->data.instance);

    Camera_DCMI->data.instance.State = HAL_DCMI_STATE_BUSY;

    __HAL_DCMI_ENABLE(&Camera_DCMI->data.instance);

    Camera_DCMI->data.instance.Instance->CR &= ~(DCMI_CR_CM);
    Camera_DCMI->data.instance.Instance->CR |= (uint32_t)(DCMI_MODE_SNAPSHOT);

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
            (middle_buffer_words >= final_buffer_words) &&
            (middle_buffer_words <= mdma_single_block_max_words || final_buffer_words <= mdma_single_block_max_words);

        Camera_DCMI->data.single_buffer_fine = single_buffer_is_fine;
        if (single_buffer_is_fine) { // single buffer is fine
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
            Camera_DCMI->data.double_buffer_first          = double_buffer_first;
            Camera_DCMI->data.double_buffer_second         = double_buffer_second;
            Camera_DCMI->data.half_middle_buffer_words     = half_middle_buffer_words;
            Camera_DCMI->data.double_buffer_transmit_count = double_buffer_transmit_count;
            Camera_DCMI->data.per_block_word               = per_block_word;
            Camera_DCMI->data.block_count                  = block_count;
            Camera_DCMI->data.final_buffer                 = final_buffer;
            Camera_DCMI->data.final_buffer_len             = final_buffer_len;
            Camera_DCMI->data.dma_not_full                 = dma_not_full;
            Camera_DCMI->the_node_list.resize(double_buffer_transmit_count - 1);
            debug("Double Buffer Transmit Count: %u\n", double_buffer_transmit_count - 1);

            HAL_MDMA_DeInit(&Camera_DCMI->data.second_stage_dma);
            HAL_MDMA_Init(&Camera_DCMI->data.second_stage_dma);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->data.second_stage_dma, HAL_MDMA_XFER_REPBLOCKCPLT_CB_ID, camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->data.second_stage_dma, HAL_MDMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_MDMA_Cplt_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->data.second_stage_dma, HAL_MDMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_MDMA_Error_Cb);
            HAL_MDMA_RegisterCallback(&Camera_DCMI->data.second_stage_dma, HAL_MDMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_MDMA_Abort_Cb);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->data.second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->data.first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            uintptr_t             destination = final_buffer + (per_block_word << 2u);
            bool                  is_second   = false;
            MDMA_LinkNodeTypeDef *prev_node   = nullptr;
            for (auto &x : Camera_DCMI->the_node_list) {
                MDMA_LinkNodeConfTypeDef node_conf;
                node_conf.Init = Camera_DCMI->data.second_stage_dma.Init;
                if (&x == &(Camera_DCMI->the_node_list.back())) {
                    node_conf.Init.Request = MDMA_REQUEST_SW;
                }
                node_conf.BlockCount             = block_count;
                node_conf.BlockDataLength        = (per_block_word << 2u);
                node_conf.DstAddress             = destination;
                node_conf.SrcAddress             = is_second ? double_buffer_second : double_buffer_first;
                node_conf.PostRequestMaskAddress = uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->data.first_stage_dma.Instance))->LIFCR));
                node_conf.PostRequestMaskData    = 0b10000u;

                HAL_MDMA_LinkedList_CreateNode(&x, &node_conf);
                if (HAL_MDMA_LinkedList_AddNode(&Camera_DCMI->data.second_stage_dma, &x, prev_node) != HAL_OK) {
                    debug("Failed to add LinkList node\n");
                    return HAL_ERROR;
                }

                prev_node = &x;
                is_second = !is_second;
                destination += (per_block_word << 2u);
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
            HAL_MDMA_Start_IT(&Camera_DCMI->data.second_stage_dma,
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
            HAL_DMA_RegisterCallback(&(Camera_DCMI->data.first_stage_dma), HAL_DMA_XFER_CPLT_CB_ID, camera_RGB_YCbCr_DMA_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->data.first_stage_dma), HAL_DMA_XFER_M1CPLT_CB_ID, camera_RGB_YCbCr_DMA_M1_Cplt_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->data.first_stage_dma), HAL_DMA_XFER_ERROR_CB_ID, camera_RGB_YCbCr_DMA_Error_Cb);
            HAL_DMA_RegisterCallback(&(Camera_DCMI->data.first_stage_dma), HAL_DMA_XFER_ABORT_CB_ID, camera_RGB_YCbCr_DMA_Abort_Cb);
            HAL_DMAEx_MultiBufferStart_IT(&(Camera_DCMI->data.first_stage_dma),
                                          (uint32_t)&(Camera_DCMI->data.instance.Instance->DR),
                                          (uint32_t)double_buffer_first,
                                          (uint32_t)double_buffer_second,
                                          (uint32_t)half_middle_buffer_words);
        }

        break;
    }
    case camera_format::format_JPEG: {
        break;
    }
    }

    Camera_DCMI->data.instance.State = HAL_DCMI_STATE_READY;

    __HAL_DCMI_ENABLE_IT(&Camera_DCMI->data.instance, DCMI_IT_FRAME);
    Camera_DCMI->data.instance.Instance->CR |= DCMI_CR_CAPTURE;
    __HAL_UNLOCK(&(Camera_DCMI->data.instance));

    return HAL_OK;
}

static uint32_t camera_RGB_YCbCr_capture_process(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    EventBits_t message;
    uint32_t    ret = 0;
    message         = xEventGroupWaitBits(Camera_DCMI->data.eg, CAMERA_CAPTURE_ALL_MASK, pdTRUE, pdFALSE, 0);
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

static HAL_StatusTypeDef camera_RGB_YCbCr_capture_resume(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    __HAL_LOCK(&Camera_DCMI->data.instance);

    Camera_DCMI->data.instance.State = HAL_DCMI_STATE_BUSY;

    __HAL_DCMI_ENABLE(&Camera_DCMI->data.instance);

    Camera_DCMI->data.instance.Instance->CR &= ~(DCMI_CR_CM);
    Camera_DCMI->data.instance.Instance->CR |= (uint32_t)(DCMI_MODE_SNAPSHOT);

    switch (target_format) {
    case camera_format::format_RGB:
    case camera_format::format_YCbCr: {

        if (Camera_DCMI->data.single_buffer_fine) { // single buffer is fine
        }
        else { // double buffer!
            HAL_MDMA_Init(&Camera_DCMI->data.second_stage_dma);
            HAL_MDMA_ConfigPostRequestMask(&Camera_DCMI->data.second_stage_dma,
                                           uint32_t(&(((DMA_TypeDef *)(Camera_DCMI->data.first_stage_dma.Instance))->LIFCR)),
                                           0b10000u);

            MDMA_LinkNodeTypeDef *prev_node = nullptr;
            for (auto &x : Camera_DCMI->the_node_list) {
                if (HAL_MDMA_LinkedList_AddNode(&Camera_DCMI->data.second_stage_dma, &x, prev_node) != HAL_OK) {
                    debug("Failed to add LinkList node\n");
                    return HAL_ERROR;
                }
                prev_node = &x;
            }

            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->the_node_list.data(),
                                              int32_t(Camera_DCMI->the_node_list.size() * sizeof(MDMA_LinkNodeConfTypeDef)));
            MYSCB_CleanInvalidateDCache_by_Addr((uint32_t *)Camera_DCMI->data.final_buffer, (int32_t)Camera_DCMI->data.final_buffer_len);
            HAL_MDMA_Start_IT(&Camera_DCMI->data.second_stage_dma,
                              (uint32_t)Camera_DCMI->data.double_buffer_first,
                              (uint32_t)Camera_DCMI->data.final_buffer,
                              (uint32_t)(Camera_DCMI->data.per_block_word << 2u),
                              (uint32_t)Camera_DCMI->data.block_count);

            taskENTER_CRITICAL();
            {
                target_dcmi_is_ok = true;
            }
            taskEXIT_CRITICAL();

            // start dma and mdma
            HAL_DMAEx_MultiBufferStart_IT(&(Camera_DCMI->data.first_stage_dma),
                                          (uint32_t)&(Camera_DCMI->data.instance.Instance->DR),
                                          (uint32_t)Camera_DCMI->data.double_buffer_first,
                                          (uint32_t)Camera_DCMI->data.double_buffer_second,
                                          (uint32_t)Camera_DCMI->data.half_middle_buffer_words);
        }

        break;
    }
    case camera_format::format_JPEG: {
        break;
    }
    }

    Camera_DCMI->data.instance.State = HAL_DCMI_STATE_READY;

    __HAL_DCMI_ENABLE_IT(&Camera_DCMI->data.instance, DCMI_IT_FRAME);
    Camera_DCMI->data.instance.Instance->CR |= DCMI_CR_CAPTURE;
    __HAL_UNLOCK(&(Camera_DCMI->data.instance));

    return HAL_OK;
}

static void camera_RGB_YCbCr_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    HAL_DMA_Abort_IT(&Camera_DCMI->data.first_stage_dma);
    if (Camera_DCMI->data.dma_not_full)
        HAL_MDMA_GenerateSWRequest(&Camera_DCMI->data.second_stage_dma);
    __HAL_DCMI_DISABLE(&Camera_DCMI->data.instance);
}

static void camera_RGB_YCbCr_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format) {
    HAL_DMA_Abort(&Camera_DCMI->data.first_stage_dma);
    HAL_MDMA_Abort(&Camera_DCMI->data.second_stage_dma);
    xEventGroupClearBits(Camera_DCMI->data.eg, 0x00ffffff);
    xQueueReset(camera_new_message);

    taskENTER_CRITICAL();
    {
        target_dcmi_is_ok = false;
    }
    taskEXIT_CRITICAL();
}

static void camera_RGB_YCbCr_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] camera_RGB_YCbCr_DMA_Cplt_Cb\n");
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug("[ISR] camera_RGB_YCbCr_DMA_M1_Cplt_Cb\n");
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_M1_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_DMA_Error_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] camera_RGB_YCbCr_DMA_Error_Cb\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_DMA_Abort_Cb(DMA_HandleTypeDef *hdma) {
    Camera_DCMI_Data *p = container_of(hdma, Camera_DCMI_Data, first_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] camera_RGB_YCbCr_DMA_Abort_Cb\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, FIRST_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_GREEN "[ISR] camera_RGB_YCbCr_MDMA_Cplt_Cb\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug("[ISR] camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb\n");
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_REPEAT_CPLT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_RED "[ISR] camera_RGB_YCbCr_MDMA_Error_Cb\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

static void camera_RGB_YCbCr_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma) {
    Camera_DCMI_Data *p = container_of(hmdma, Camera_DCMI_Data, second_stage_dma);
    //
    debug(ANSI_COLOR_FG_CYAN "[ISR] camera_RGB_YCbCr_MDMA_Abort_Cb\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, SECOND_STAGE_DMA_ABORT, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    Camera_DCMI_Data *p = container_of(hdcmi, Camera_DCMI_Data, instance);
    //
    debug(ANSI_COLOR_FG_YELLOW "[ISR] HAL_DCMI_FrameEventCallback\n" ANSI_COLOR_RESET);
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, CAMERA_CAPTURE_OK, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi) {
    Camera_DCMI_Data *p = container_of(hdcmi, Camera_DCMI_Data, instance);
    //
    debug("[ISR] HAL_DCMI_ErrorCallback\n");
    BaseType_t should_yield = pdFALSE;
    xEventGroupSetBitsFromISR(p->eg, CAMERA_CAPTURE_ERROR, &should_yield);
    xSemaphoreGiveFromISR(camera_new_message, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&(target_dcmi->data.first_stage_dma));
}

static void MY_HAL_DCMI_IRQHandler(DCMI_HandleTypeDef *hdcmi);
void        DCMI_IRQHandler(void) {
    MY_HAL_DCMI_IRQHandler(&(target_dcmi->data.instance));
}

void camera_mdma_IRQHandler() {
    if (target_dcmi_is_ok) {
        HAL_MDMA_IRQHandler(&target_dcmi->data.second_stage_dma);
    }
}

static void MY_HAL_DCMI_IRQHandler(DCMI_HandleTypeDef *hdcmi) {
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

static void calculate_decompose(size_t &x, size_t &y, size_t &z, size_t y_max) {
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

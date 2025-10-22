#define FILE_DEBUG 1

#include "camera_declare.hpp"

// variables
Camera_DCMI_HandleType  RGB_hdcmi {};   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
Camera_DCMI_HandleType  YCbCr_hdcmi {}; // YCbCr(4:2:2) => Encode Inside(JPEG)
Camera_DCMI_HandleType  JPEG_hdcmi {};  // JPEG(4:2:2)
Camera_DCMI_HandleType *target_dcmi {};
bool                    target_dcmi_is_ok = false;

// OV5640_IO_t             ov5640_io {};
// OV5640_Object_t         ov5640 {};

sensor_t          ov5640_sensor;
soft_sccb_handle  my_sccb {};

framesize_t       current_resolution {};
pixformat_t       current_format {};

SemaphoreHandle_t camera_interface_changed {};
SemaphoreHandle_t camera_interface_restart {};
bool              camera_deinit_have_done = true;

lv_obj_t         *screen_container {};
lv_obj_t         *camera_capture_image {};
lv_obj_t         *button_container {};
lv_obj_t         *take_photo_button {};
lv_obj_t         *change_to_file_explorer_button {};
lv_obj_t         *open_setting_button {};
lv_obj_t         *take_photo_label {};
lv_obj_t         *change_to_file_explorer_label {};
lv_obj_t         *open_setting_label {};
lv_obj_t         *indicator_label {};

uint8_t D2_SRAM[128 * 1024] IN_SRAM2 __ALIGNED(32);

// static variables

static char error_message[128] {};
static FIL  picture_file {};

// functions

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
    dcmi_capture_resource_init();

    xSemaphoreGive(sema_camera_routine_init_done);

    bool          queue_enable        = false;
    bool          can_catch_scene     = false;
    bool          camera_captured_end = false;
    QueueHandle_t the_queue {};
    framesize_t   resolution {};
    pixformat_t   format {};
    uint32_t      data_length {}, src_w {}, src_h {};
    uint32_t      use_full_buffer = 1;
    bool          screen_RGB_mode = true;

    while (1) {
        // ?
        the_queue = xQueueSelectFromSet(camera_queue_set, portMAX_DELAY);

        if (the_queue == camera_interface_changed || the_queue == camera_interface_restart) {
            xSemaphoreTake(camera_interface_changed, 0);
            if (!screen_RGB_mode)
                continue;

            current_resolution = FRAMESIZE_QVGA;
            current_format     = PIXFORMAT_RGB565;
            resolution_parse(resolution, data_length, src_w, src_h, format);

            camera_deinit_have_done = false;
            if (camera_init(can_catch_scene, resolution, format, false) != 0) {
                continue;
            }

            queue_enable = true;

            if (can_catch_scene) {
                camera_captured_end = false;
                camera_start_capture(target_dcmi, current_format,
                                     (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                     (uintptr_t)jpeg_before_buffer_rgb, data_length);
            }
        }
        else if (the_queue == camera_new_message && can_catch_scene) {
            xSemaphoreTake(camera_new_message, 0);
            if (!queue_enable)
                continue;

            auto ret = camera_capture_process(target_dcmi, current_format);

            if ((ret & (uint32_t)message_process::ERROR_STOP)) {
                camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
                camera_deinit(error_message, nullptr);

                can_catch_scene     = false;
                camera_captured_end = false;
                screen_RGB_mode     = true;
                queue_enable        = false;

                xSemaphoreGive(camera_interface_restart);
                continue;
            }

            if (ret & (uint32_t)message_process::CAPTURE_END) {
                camera_captured_end = true;

                if (screen_RGB_mode)
                    camera_RGB_YCbCr_capture_abort_first_stage_dma(target_dcmi, current_format);
                else
                    camera_JPEG_capture_abort_first_stage_dma(target_dcmi, current_format);
            }

            if (ret & (uint32_t)message_process::DATA_END) {
                if (!camera_captured_end) {
                    debug("It seems not right\n");
                }

                if (screen_RGB_mode) {
                    camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
                    camera_captured_end = false;

                    MYSCB_InvalidateDCache_by_Addr((void *)jpeg_before_buffer_rgb, (int32_t)data_length);
                    full_pict_show_size[0] = MY_DISP_HOR_RES;
                    full_pict_show_size[1] = MY_DISP_VER_RES;

                    if (use_full_buffer == 1) {
                        picture_scaling(jpeg_before_buffer_rgb, full_screen_pict_show_buffer, src_w, src_h,
                                        full_pict_show_size[0], full_pict_show_size[1]);
                        img_dsc.data    = full_screen_pict_show_buffer;
                        use_full_buffer = 2;
                    }
                    else {
                        picture_scaling(jpeg_before_buffer_rgb, full_screen_pict_show_buffer_2, src_w, src_h,
                                        full_pict_show_size[0], full_pict_show_size[1]);
                        img_dsc.data    = full_screen_pict_show_buffer_2;
                        use_full_buffer = 1;
                    }

                    img_dsc.header.w  = full_pict_show_size[0];
                    img_dsc.header.h  = full_pict_show_size[1];
                    img_dsc.data_size = full_pict_show_size[0] * full_pict_show_size[1] * 2;

                    screen_image_operate(&img_dsc);
                    camera_capture_resume(target_dcmi, current_format);
                }
                else {
                    camera_JPEG_capture_stop(target_dcmi, current_format);
                    if (sdcard_is_mounted) {
                        size_t length = (target_dcmi->jpeg_data_count_calculate << 2u);
                        MYSCB_InvalidateDCache_by_Addr((void *)jpeg_before_buffer_rgb, (int32_t)length);

                        // storage to file
                        FRESULT f_res       = FR_OK;
                        UINT    write_bytes = 0;
                        f_res               = f_open(&picture_file, "0:/1.jpeg", FA_WRITE | FA_CREATE_ALWAYS);
                        if (f_res == FR_OK) {
                            f_res = f_write(&picture_file, (void *)jpeg_before_buffer_rgb, length, &write_bytes);
                            if (f_res == FR_OK && (length == write_bytes)) {
                                f_res = f_close(&picture_file);
                                if (f_res == FR_OK) {
                                    // pass
                                }
                                else {
                                    indicator_operate("Failed to save Picture!");
                                    vTaskDelay(pdMS_TO_TICKS(500));
                                }
                            }
                            else {
                                indicator_operate("Failed to save Picture!");
                                vTaskDelay(pdMS_TO_TICKS(500));
                            }
                        }
                        else {
                            indicator_operate("Failed to Open SD Card File!");
                            vTaskDelay(pdMS_TO_TICKS(500));
                        }

                        // indicator and others..
                        indicator_operate(nullptr);
                    }
                    else {
                        indicator_operate("SD Card Not Inserted!");
                        vTaskDelay(pdMS_TO_TICKS(500));
                        indicator_operate(nullptr);
                    }

                    // change to RGB
                    current_resolution = FRAMESIZE_QVGA;
                    current_format     = PIXFORMAT_RGB565;
                    resolution_parse(resolution, data_length, src_w, src_h, format);

                    camera_deinit_have_done = false;
                    if (camera_init(can_catch_scene, resolution, format, true) != 0) {
                        xSemaphoreGive(camera_interface_restart);
                        continue;
                    }
                    queue_enable        = true;
                    camera_captured_end = false;
                    screen_RGB_mode     = true;
                    camera_start_capture(target_dcmi, current_format,
                                         (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                         (uintptr_t)jpeg_before_buffer_rgb, data_length); // RGB Capture
                }
            }
        }
        else if (the_queue == camera_exit) {
            xSemaphoreTake(camera_exit, 0);
            queue_enable = false;

            camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
            camera_deinit(nullptr, nullptr);
            vTaskDelay(pdMS_TO_TICKS(20));

            send_command_to_main_manage(nullptr, 0, manage_command_type::to_file_explorer, portMAX_DELAY);
        }
        else if (the_queue == camera_error) {
            xSemaphoreTake(camera_error, 0);

            camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
            camera_deinit(error_message, nullptr);

            can_catch_scene = false;
        }
        else if (the_queue == camera_take_photo) {
            xSemaphoreTake(camera_take_photo, 0);

            if (screen_RGB_mode) {
                camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
                screen_RGB_mode = false;
            }

            current_resolution = FRAMESIZE_QSXGA;
            current_format     = PIXFORMAT_JPEG;
            resolution_parse(resolution, data_length, src_w, src_h, format);

            camera_deinit_have_done = false;

            ov5640_sensor.set_quality(&ov5640_sensor, 0x0A);
            if (camera_init(can_catch_scene, resolution, format, true) != 0) {
                xSemaphoreGive(camera_interface_restart);
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            queue_enable        = true;
            camera_captured_end = false;
            camera_start_capture(target_dcmi, current_format,
                                 (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                 (uintptr_t)jpeg_before_buffer_rgb, 16 * 1024 * 1024); // JPEG Capture
        }
    }
}


void lvgl_create_camera_interface() {
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


void change_to_file_explorer_callback(lv_event_t *e) {
    xSemaphoreGive(camera_exit);
}
void take_photo_callback(lv_event_t *e) {
    xSemaphoreGive(camera_take_photo);
}
void open_setting_callback(lv_event_t *e) {}

void indicator_operate(const char *message) {
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

void screen_image_operate(void *source) {
    lv_lock();
    {
        lv_image_set_src(camera_capture_image, source);
        if (source) {
            lv_image_set_align(camera_capture_image, LV_IMAGE_ALIGN_CENTER);
        }
    }
    lv_unlock();
}

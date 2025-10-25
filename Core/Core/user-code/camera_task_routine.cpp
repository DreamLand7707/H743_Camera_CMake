#define FILE_DEBUG 1

#include "camera_declare.hpp"
#include <sys/times.h>
#include <random>

// variables
Camera_DCMI_HandleType  RGB_hdcmi {};   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
Camera_DCMI_HandleType  YCbCr_hdcmi {}; // YCbCr(4:2:2) => Encode Inside(JPEG)
Camera_DCMI_HandleType  JPEG_hdcmi {};  // JPEG(4:2:2)
Camera_DCMI_HandleType *target_dcmi {};
bool                    target_dcmi_is_ok = false;

OV5640_IO_t             ov5640_io {};
OV5640_Object_t         ov5640 {};
soft_sccb_handle        my_sccb {};

camera_resolution       current_resolution {};
camera_format           current_format {};

SemaphoreHandle_t       camera_interface_changed {};
SemaphoreHandle_t       camera_interface_restart {};
bool                    camera_deinit_have_done = true;

lv_obj_t               *screen_container {};
lv_obj_t               *camera_capture_image {};
lv_obj_t               *button_container {};
lv_obj_t               *take_photo_button {};
lv_obj_t               *change_to_file_explorer_button {};
lv_obj_t               *open_setting_button {};
lv_obj_t               *take_photo_label {};
lv_obj_t               *change_to_file_explorer_label {};
lv_obj_t               *open_setting_label {};
lv_obj_t               *indicator_label {};

uint8_t D2_SRAM[128 * 1024] IN_SRAM2 __ALIGNED(32);

// static variables

static char                                    error_message[128] {};
static char                                    file_name[128] {};
static FIL                                     picture_file {};
static tms                                     t {};

static std::default_random_engine              eng;
static std::uniform_int_distribution<uint32_t> unif_name(0, UINT32_MAX);

// functions
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

namespace
{
    lv_obj_t *camera_settings_screen;
    lv_obj_t *camera_settings_container;

    lv_obj_t *camera_settings_indicator_container;
    lv_obj_t *camera_settings_indicator_text_box;
    lv_obj_t *camera_settings_indicator_text;
    lv_obj_t *camera_settings_indicator_return_btn;
    lv_obj_t *camera_settings_indicator_return_btn_label;

    lv_obj_t *camera_settings_functions_container;

    lv_obj_t *camera_settings_strobe_setting_box;
    lv_obj_t *camera_settings_strobe_setting_box_text_box;
    lv_obj_t *camera_settings_strobe_setting_box_text;
    lv_obj_t *camera_settings_strobe_setting_box_btns_box;
    lv_obj_t *camera_settings_strobe_settings_btns[4];

    lv_obj_t *camera_settings_picture_size_box;
    lv_obj_t *camera_settings_picture_size_box_text_box;
    lv_obj_t *camera_settings_picture_size_box_text;
    lv_obj_t *camera_settings_picture_size_box_list_spacer;
    lv_obj_t *camera_settings_picture_size_box_list;
} // namespace

void lvgl_create_setting_interface() {

    static const char *options = "QXGA\n"
                                 "SXGA\n"
                                 "XGA\n"
                                 "VGA";

    camera_settings_screen     = lv_obj_create(nullptr);

    camera_settings_container  = lv_obj_create(camera_settings_screen);
    lv_obj_set_style_size(camera_settings_container, LV_PCT(100), LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(camera_settings_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(camera_settings_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(camera_settings_container, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_radius(camera_settings_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_layout(camera_settings_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(camera_settings_container, LV_FLEX_FLOW_COLUMN);
    {
        camera_settings_indicator_container = lv_obj_create(camera_settings_container);
        lv_obj_set_style_width(camera_settings_indicator_container, LV_PCT(100), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(camera_settings_indicator_container, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(camera_settings_indicator_container, 8, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(camera_settings_indicator_container, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(camera_settings_indicator_container, 0, LV_STATE_DEFAULT);
        lv_obj_set_layout(camera_settings_indicator_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(camera_settings_indicator_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_flex_grow(camera_settings_indicator_container, 1, LV_STATE_DEFAULT);

        camera_settings_indicator_text_box = lv_obj_create(camera_settings_indicator_container);
        lv_obj_set_style_height(camera_settings_indicator_text_box, LV_PCT(100), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(camera_settings_indicator_text_box, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(camera_settings_indicator_text_box, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(camera_settings_indicator_text_box, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(camera_settings_indicator_text_box, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_flex_grow(camera_settings_indicator_text_box, 9, LV_STATE_DEFAULT);
        lv_obj_remove_flag(camera_settings_indicator_text_box, LV_OBJ_FLAG_SCROLLABLE);

        camera_settings_indicator_text = lv_label_create(camera_settings_indicator_text_box);
        lv_obj_set_style_text_color(camera_settings_indicator_text, lv_color_black(), LV_STATE_DEFAULT);
        lv_label_set_text_static(camera_settings_indicator_text, "Camera Settings");
        lv_obj_set_align(camera_settings_indicator_text, LV_ALIGN_CENTER);
        lv_obj_set_style_text_font(camera_settings_indicator_text, &lv_font_montserrat_18, LV_STATE_DEFAULT);

        camera_settings_indicator_return_btn = lv_btn_create(camera_settings_indicator_container);
        lv_obj_set_style_height(camera_settings_indicator_return_btn, LV_PCT(100), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(camera_settings_indicator_return_btn, lv_color_hex(0x34a3e7), LV_STATE_DEFAULT);
        lv_obj_set_style_flex_grow(camera_settings_indicator_return_btn, 1, LV_STATE_DEFAULT);
        lv_obj_remove_flag(camera_settings_indicator_return_btn, LV_OBJ_FLAG_SCROLLABLE);

        camera_settings_indicator_return_btn_label = lv_label_create(camera_settings_indicator_return_btn);
        lv_obj_set_align(camera_settings_indicator_return_btn_label, LV_ALIGN_CENTER);
        lv_obj_set_style_text_color(camera_settings_indicator_return_btn_label, lv_color_black(), LV_STATE_DEFAULT);
        lv_label_set_text_static(camera_settings_indicator_return_btn_label, LV_SYMBOL_LEFT);
    }

    {
        camera_settings_functions_container = lv_obj_create(camera_settings_container);
        lv_obj_set_style_width(camera_settings_functions_container, LV_PCT(100), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(camera_settings_functions_container, 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(camera_settings_functions_container, 8, LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(camera_settings_functions_container, lv_color_white(), LV_STATE_DEFAULT);
        lv_obj_set_style_radius(camera_settings_functions_container, 0, LV_STATE_DEFAULT);
        lv_obj_set_layout(camera_settings_functions_container, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(camera_settings_functions_container, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_flex_grow(camera_settings_functions_container, 4, LV_STATE_DEFAULT);
        lv_obj_add_flag(camera_settings_functions_container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

        {
            camera_settings_strobe_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_strobe_setting_box, LV_PCT(90), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_strobe_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_strobe_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_strobe_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_strobe_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_strobe_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_strobe_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_strobe_setting_box, 1, LV_STATE_DEFAULT);

            {
                camera_settings_strobe_setting_box_text_box = lv_obj_create(camera_settings_strobe_setting_box);
                lv_obj_set_style_width(camera_settings_strobe_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_strobe_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_strobe_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_strobe_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_strobe_setting_box_text = lv_label_create(camera_settings_strobe_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_strobe_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_strobe_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_strobe_setting_box_text, "strobe setting");
            }

            {
                camera_settings_strobe_setting_box_btns_box = lv_obj_create(camera_settings_strobe_setting_box);
                lv_obj_set_style_width(camera_settings_strobe_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_strobe_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_strobe_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_strobe_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_strobe_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_strobe_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_strobe_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

                {
                    for (int i = 0; i < 4; i++) {
                        camera_settings_strobe_settings_btns[i] = lv_checkbox_create(camera_settings_strobe_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_strobe_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_strobe_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_strobe_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_strobe_settings_btns[0], "close");
                    lv_checkbox_set_text_static(camera_settings_strobe_settings_btns[1], "always");
                    lv_checkbox_set_text_static(camera_settings_strobe_settings_btns[2], "once");
                    lv_checkbox_set_text_static(camera_settings_strobe_settings_btns[3], "after shot");
                }
            }
        }

        {
            camera_settings_picture_size_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_picture_size_box, LV_PCT(90), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_picture_size_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_picture_size_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_picture_size_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_picture_size_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_picture_size_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_picture_size_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_picture_size_box, 1, LV_STATE_DEFAULT);

            {
                camera_settings_picture_size_box_text_box = lv_obj_create(camera_settings_picture_size_box);
                lv_obj_set_style_width(camera_settings_picture_size_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_picture_size_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_picture_size_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_picture_size_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_picture_size_box_text = lv_label_create(camera_settings_picture_size_box_text_box);
                lv_obj_set_style_align(camera_settings_picture_size_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_picture_size_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_picture_size_box_text, "output file size");
            }

            {
                camera_settings_picture_size_box_list_spacer = lv_obj_create(camera_settings_picture_size_box);
                lv_obj_set_style_width(camera_settings_picture_size_box_list_spacer, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_picture_size_box_list_spacer, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_picture_size_box_list_spacer, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_picture_size_box_list_spacer, 4, LV_STATE_DEFAULT);

                camera_settings_picture_size_box_list = lv_dropdown_create(camera_settings_picture_size_box_list_spacer);
                lv_obj_set_style_size(camera_settings_picture_size_box_list, LV_PCT(100), LV_PCT(33), LV_STATE_DEFAULT);
                lv_dropdown_set_options_static(camera_settings_picture_size_box_list, options);
            }
        }
    }

    lv_obj_set_user_data(camera_settings_strobe_setting_box_btns_box, camera_settings_strobe_settings_btns[0]);
    lv_obj_add_state(camera_settings_strobe_settings_btns[0], LV_STATE_CHECKED);

    lv_obj_add_event_cb(camera_settings_indicator_return_btn, indicator_return_btn_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_strobe_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
}


void change_to_file_explorer_callback(lv_event_t *e) {
    xSemaphoreGive(camera_exit);
}
void take_photo_callback(lv_event_t *e) {
    xSemaphoreGive(camera_take_photo);
}
void open_setting_callback(lv_event_t *e) {
    lv_screen_load(camera_settings_screen);
}
void indicator_return_btn_callback(lv_event_t *e) {
    lv_screen_load(camera_screen);
}

void checkboxs_callback(lv_event_t *e) {
    lv_obj_t *target       = lv_event_get_target_obj(e);
    lv_obj_t *curr_target  = lv_event_get_current_target_obj(e);

    auto      prev_checked = (lv_obj_t *)lv_obj_get_user_data(curr_target);

    if (curr_target == target) {
        return;
    }

    lv_obj_set_user_data(curr_target, target);
    lv_obj_remove_state(prev_checked, LV_STATE_CHECKED);
    lv_obj_add_state(target, LV_STATE_CHECKED);

    xSemaphoreGive(camera_strobe_setting_changed);
}

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
    lvgl_create_setting_interface();
    dcmi_capture_resource_init();

    xSemaphoreGive(sema_camera_routine_init_done);

    bool          queue_enable        = false;
    bool          can_catch_scene     = false;
    bool          camera_captured_end = false;
    QueueHandle_t the_queue {};
    uint32_t      resolution {}, format {}, data_length {}, src_w {}, src_h {};
    uint32_t      use_full_buffer = 1;
    bool          screen_RGB_mode = true;
    uint32_t      strobe_state    = 0;

    while (1) {
        // ?
        the_queue = xQueueSelectFromSet(camera_queue_set, portMAX_DELAY);

        if (the_queue == camera_strobe_setting_changed) {
            uint32_t new_strobe_state;
            xSemaphoreTake(camera_strobe_setting_changed, 0);
            lv_lock();
            {
                new_strobe_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_strobe_setting_box_btns_box));
            }
            lv_unlock();
            if (new_strobe_state == 0) {
                OV5640_STROBE(0);
                strobe_state = 0;
            }
            else if (new_strobe_state == 1) {
                OV5640_STROBE(1);
                strobe_state = 1;
            }
            else if (new_strobe_state == 2) {
                OV5640_STROBE(0);
                strobe_state = 2;
            }
            else if (new_strobe_state == 3) {
                OV5640_STROBE(1);
                strobe_state = 3;
            }
        }
        else if (the_queue == camera_interface_changed || the_queue == camera_interface_restart) {
            xSemaphoreTake(camera_interface_changed, 0);
            if (!screen_RGB_mode)
                continue;

            current_resolution = camera_resolution::reso_QVGA;
            current_format     = camera_format::format_RGB;
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
                else {
                    camera_JPEG_capture_abort_first_stage_dma(target_dcmi, current_format);
                    if (strobe_state == 2 && target_dcmi->jpeg_use_strobe) {
                        OV5640_STROBE(0);
                        target_dcmi->jpeg_use_strobe = false;
                    }
                    else if (strobe_state == 3) {
                        OV5640_STROBE(0);
                        target_dcmi->jpeg_use_strobe = false;
                        //
                        strobe_state = 0;
                        lv_lock();
                        {
                            lv_obj_t *obj_have_checked = (lv_obj_t *)lv_obj_get_user_data(camera_settings_strobe_setting_box_btns_box);
                            lv_obj_remove_state(obj_have_checked, LV_STATE_CHECKED);
                            lv_obj_add_state(camera_settings_strobe_settings_btns[0], LV_STATE_CHECKED);
                            lv_obj_set_user_data(camera_settings_strobe_setting_box_btns_box, camera_settings_strobe_settings_btns[0]);
                        }
                        lv_unlock();
                    }
                }
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

                        eng.seed(times(&t));
                        do {
                            FILINFO fno;
                            sprintf(file_name, "0:/%X.jpeg", (unsigned)unif_name(eng));
                            f_res = f_stat(file_name, &fno);
                        }
                        while (f_res == FR_OK);

                        f_res = f_open(&picture_file, file_name, FA_WRITE | FA_CREATE_NEW);
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
                    current_resolution = camera_resolution::reso_QVGA;
                    current_format     = camera_format::format_RGB;
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

            uint32_t select_index = lv_dropdown_get_selected(camera_settings_picture_size_box_list);
            switch (select_index) {
            case 0: {
                current_resolution = camera_resolution::reso_QXGA;
                break;
            }
            case 1: {
                current_resolution = camera_resolution::reso_SXGA;
                break;
            }
            case 2: {
                current_resolution = camera_resolution::reso_XGA;
                break;
            }
            case 3: {
                current_resolution = camera_resolution::reso_VGA;
                break;
            }
            default:
                break;
            }

            current_format = camera_format::format_JPEG;
            resolution_parse(resolution, data_length, src_w, src_h, format);

            camera_deinit_have_done = false;
            if (camera_init(can_catch_scene, resolution, format, true) != 0) {
                xSemaphoreGive(camera_interface_restart);
                continue;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
            queue_enable        = true;
            camera_captured_end = false;
            if (strobe_state == 2) {
                target_dcmi->jpeg_use_strobe = true;
            }
            else {
                target_dcmi->jpeg_use_strobe = false;
            }
            camera_start_capture(target_dcmi, current_format,
                                 (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                 (uintptr_t)jpeg_before_buffer_rgb, 16 * 1024 * 1024); // JPEG Capture
        }
    }
}

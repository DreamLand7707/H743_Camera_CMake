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

TimerHandle_t           camera_single_focus_timer;
TimerHandle_t           camera_constant_focus_timer;

lv_obj_t               *screen_container {};
lv_obj_t               *camera_capture_image {};
lv_obj_t               *camera_capture_image_eventor {};
lv_obj_t               *button_container {};
lv_obj_t               *take_photo_button {};
lv_obj_t               *change_to_file_explorer_button {};
lv_obj_t               *open_setting_button {};
lv_obj_t               *take_photo_label {};
lv_obj_t               *change_to_file_explorer_label {};
lv_obj_t               *open_setting_label {};
lv_obj_t               *indicator_label {};

uint8_t D2_SRAM[128 * 1024] IN_SRAM2 __ALIGNED(32);

void (*shot_btn_isr_callback)();

// static variables

static char                                    error_message[128] {};
static char                                    file_name[128] {};
static FIL                                     picture_file {};
static tms                                     t {};

static std::default_random_engine              eng;
static std::uniform_int_distribution<uint32_t> unif_name(0, UINT32_MAX);

//
static void set_shot_btn();

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

    camera_capture_image_eventor = lv_obj_create(camera_capture_image);
    lv_obj_set_style_size(camera_capture_image_eventor, LV_PCT(100), LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(camera_capture_image_eventor, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(camera_capture_image_eventor, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_opa(camera_capture_image_eventor, 0, LV_STATE_DEFAULT);

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
    lv_obj_add_event_cb(camera_capture_image_eventor, image_eventor_callback, LV_EVENT_CLICKED, nullptr);

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

    lv_obj_t *camera_settings_light_mode_setting_box;
    lv_obj_t *camera_settings_light_mode_setting_box_text_box;
    lv_obj_t *camera_settings_light_mode_setting_box_text;
    lv_obj_t *camera_settings_light_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_light_mode_settings_btns[5];

    lv_obj_t *camera_settings_effect_mode_setting_box;
    lv_obj_t *camera_settings_effect_mode_setting_box_text_box;
    lv_obj_t *camera_settings_effect_mode_setting_box_text;
    lv_obj_t *camera_settings_effect_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_effect_mode_settings_btns[7];

    lv_obj_t *camera_settings_zoom_mode_setting_box;
    lv_obj_t *camera_settings_zoom_mode_setting_box_text_box;
    lv_obj_t *camera_settings_zoom_mode_setting_box_text;
    lv_obj_t *camera_settings_zoom_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_zoom_mode_settings_btns[4];

    lv_obj_t *camera_settings_mirror_flip_mode_setting_box;
    lv_obj_t *camera_settings_mirror_flip_mode_setting_box_text_box;
    lv_obj_t *camera_settings_mirror_flip_mode_setting_box_text;
    lv_obj_t *camera_settings_mirror_flip_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_mirror_flip_mode_settings_btns[4];

    lv_obj_t *camera_settings_colorbar_mode_setting_box;
    lv_obj_t *camera_settings_colorbar_mode_setting_box_text_box;
    lv_obj_t *camera_settings_colorbar_mode_setting_box_text;
    lv_obj_t *camera_settings_colorbar_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_colorbar_mode_settings_btns[3];

    lv_obj_t *camera_settings_brightness_mode_setting_box;
    lv_obj_t *camera_settings_brightness_mode_setting_box_text_box;
    lv_obj_t *camera_settings_brightness_mode_setting_box_text;
    lv_obj_t *camera_settings_brightness_mode_setting_roller;

    lv_obj_t *camera_settings_night_mode_setting_box;
    lv_obj_t *camera_settings_night_mode_setting_box_text_box;
    lv_obj_t *camera_settings_night_mode_setting_box_text;
    lv_obj_t *camera_settings_night_mode_setting_box_btns_box;
    lv_obj_t *camera_settings_night_mode_settings_btns[1];

    lv_obj_t *camera_settings_saturation_setting_box;
    lv_obj_t *camera_settings_saturation_setting_box_text_box;
    lv_obj_t *camera_settings_saturation_setting_box_text;
    lv_obj_t *camera_settings_saturation_setting_roller;

    lv_obj_t *camera_settings_contrast_setting_box;
    lv_obj_t *camera_settings_contrast_setting_box_text_box;
    lv_obj_t *camera_settings_contrast_setting_box_text;
    lv_obj_t *camera_settings_contrast_setting_roller;

    lv_obj_t *camera_settings_sharpness_setting_box;
    lv_obj_t *camera_settings_sharpness_setting_box_text_box;
    lv_obj_t *camera_settings_sharpness_setting_box_text;
    lv_obj_t *camera_settings_sharpness_setting_roller;
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
        lv_obj_set_flex_align(camera_settings_functions_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

        {
            camera_settings_strobe_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_strobe_setting_box, LV_PCT(90), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_strobe_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_strobe_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
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
            lv_obj_set_style_border_width(camera_settings_strobe_setting_box_btns_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_strobe_setting_box_btns_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
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
            lv_obj_set_style_border_width(camera_settings_picture_size_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_picture_size_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
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

        {
            camera_settings_light_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_light_mode_setting_box, LV_PCT(100), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_light_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_light_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_light_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_light_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_light_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_light_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_light_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_light_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_add_flag(camera_settings_light_mode_setting_box, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            {
                camera_settings_light_mode_setting_box_text_box = lv_obj_create(camera_settings_light_mode_setting_box);
                lv_obj_set_style_width(camera_settings_light_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_light_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_light_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_light_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_light_mode_setting_box_text = lv_label_create(camera_settings_light_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_light_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_light_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_light_mode_setting_box_text, "light mode");
            }

            {
                camera_settings_light_mode_setting_box_btns_box = lv_obj_create(camera_settings_light_mode_setting_box);
                lv_obj_set_style_width(camera_settings_light_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_light_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_light_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_light_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_light_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_light_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_light_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
                {
                    for (int i = 0; i < 5; i++) {
                        camera_settings_light_mode_settings_btns[i] = lv_checkbox_create(camera_settings_light_mode_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_light_mode_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_light_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_light_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_light_mode_settings_btns[0], "auto");
                    lv_checkbox_set_text_static(camera_settings_light_mode_settings_btns[1], "sunny");
                    lv_checkbox_set_text_static(camera_settings_light_mode_settings_btns[2], "office");
                    lv_checkbox_set_text_static(camera_settings_light_mode_settings_btns[3], "cloudy");
                    lv_checkbox_set_text_static(camera_settings_light_mode_settings_btns[4], "home");
                }
            }
        }

        {
            camera_settings_effect_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_effect_mode_setting_box, LV_PCT(140), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_effect_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_effect_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_effect_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_effect_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_effect_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_effect_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_effect_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_effect_mode_setting_box, 1, LV_STATE_DEFAULT);
            {
                camera_settings_effect_mode_setting_box_text_box = lv_obj_create(camera_settings_effect_mode_setting_box);
                lv_obj_set_style_width(camera_settings_effect_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_effect_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_effect_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_effect_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_effect_mode_setting_box_text = lv_label_create(camera_settings_effect_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_effect_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_effect_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_effect_mode_setting_box_text, "effect");
            }

            {
                camera_settings_effect_mode_setting_box_btns_box = lv_obj_create(camera_settings_effect_mode_setting_box);
                lv_obj_set_style_width(camera_settings_effect_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_effect_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_effect_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_effect_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_effect_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_effect_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_effect_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

                {
                    for (int i = 0; i < 7; i++) {
                        camera_settings_effect_mode_settings_btns[i] = lv_checkbox_create(camera_settings_effect_mode_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_effect_mode_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_effect_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_effect_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[0], "none");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[1], "blue");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[2], "red");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[3], "green");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[4], "bw");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[5], "sepia");
                    lv_checkbox_set_text_static(camera_settings_effect_mode_settings_btns[6], "negative");
                }
            }
        }

        {
            camera_settings_zoom_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_zoom_mode_setting_box, LV_PCT(100), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_zoom_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_zoom_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_zoom_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_zoom_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_zoom_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_zoom_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_zoom_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_zoom_mode_setting_box, 1, LV_STATE_DEFAULT);
            {
                camera_settings_zoom_mode_setting_box_text_box = lv_obj_create(camera_settings_zoom_mode_setting_box);
                lv_obj_set_style_width(camera_settings_zoom_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_zoom_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_zoom_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_zoom_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_zoom_mode_setting_box_text = lv_label_create(camera_settings_zoom_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_zoom_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_zoom_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_zoom_mode_setting_box_text, "zoom");
            }

            {
                camera_settings_zoom_mode_setting_box_btns_box = lv_obj_create(camera_settings_zoom_mode_setting_box);
                lv_obj_set_style_width(camera_settings_zoom_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_zoom_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_zoom_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_zoom_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_zoom_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_zoom_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_zoom_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
                {
                    for (int i = 0; i < 4; i++) {
                        camera_settings_zoom_mode_settings_btns[i] = lv_checkbox_create(camera_settings_zoom_mode_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_zoom_mode_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_zoom_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_zoom_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_zoom_mode_settings_btns[0], "x1");
                    lv_checkbox_set_text_static(camera_settings_zoom_mode_settings_btns[1], "x2");
                    lv_checkbox_set_text_static(camera_settings_zoom_mode_settings_btns[2], "x4");
                    lv_checkbox_set_text_static(camera_settings_zoom_mode_settings_btns[3], "x8");
                }
            }
        }

        {
            camera_settings_mirror_flip_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_mirror_flip_mode_setting_box, LV_PCT(90), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_mirror_flip_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_mirror_flip_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_mirror_flip_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_mirror_flip_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_mirror_flip_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_mirror_flip_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_mirror_flip_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_mirror_flip_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_add_flag(camera_settings_mirror_flip_mode_setting_box, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            {
                camera_settings_mirror_flip_mode_setting_box_text_box = lv_obj_create(camera_settings_mirror_flip_mode_setting_box);
                lv_obj_set_style_width(camera_settings_mirror_flip_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_mirror_flip_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_mirror_flip_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_mirror_flip_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_mirror_flip_mode_setting_box_text = lv_label_create(camera_settings_mirror_flip_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_mirror_flip_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_mirror_flip_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_mirror_flip_mode_setting_box_text, "mirror flip");
            }

            {
                camera_settings_mirror_flip_mode_setting_box_btns_box = lv_obj_create(camera_settings_mirror_flip_mode_setting_box);
                lv_obj_set_style_width(camera_settings_mirror_flip_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_mirror_flip_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_mirror_flip_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_mirror_flip_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_mirror_flip_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_mirror_flip_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_mirror_flip_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
                {
                    for (int i = 0; i < 4; i++) {
                        camera_settings_mirror_flip_mode_settings_btns[i] = lv_checkbox_create(camera_settings_mirror_flip_mode_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_mirror_flip_mode_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_mirror_flip_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_mirror_flip_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_mirror_flip_mode_settings_btns[0], "none");
                    lv_checkbox_set_text_static(camera_settings_mirror_flip_mode_settings_btns[1], "flip");
                    lv_checkbox_set_text_static(camera_settings_mirror_flip_mode_settings_btns[2], "mirror");
                    lv_checkbox_set_text_static(camera_settings_mirror_flip_mode_settings_btns[3], "mirror flip");
                }
            }
        }

        {
            camera_settings_colorbar_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_colorbar_mode_setting_box, LV_PCT(90), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_colorbar_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_colorbar_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_colorbar_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_colorbar_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_colorbar_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_colorbar_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_colorbar_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_colorbar_mode_setting_box, 1, LV_STATE_DEFAULT);
            {
                camera_settings_colorbar_mode_setting_box_text_box = lv_obj_create(camera_settings_colorbar_mode_setting_box);
                lv_obj_set_style_width(camera_settings_colorbar_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_colorbar_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_colorbar_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_colorbar_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_colorbar_mode_setting_box_text = lv_label_create(camera_settings_colorbar_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_colorbar_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_colorbar_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_colorbar_mode_setting_box_text, "color bar");
            }

            {
                camera_settings_colorbar_mode_setting_box_btns_box = lv_obj_create(camera_settings_colorbar_mode_setting_box);
                lv_obj_set_style_width(camera_settings_colorbar_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_colorbar_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_colorbar_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_colorbar_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_colorbar_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_colorbar_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_colorbar_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
                {
                    for (int i = 0; i < 3; i++) {
                        camera_settings_colorbar_mode_settings_btns[i] = lv_checkbox_create(camera_settings_colorbar_mode_setting_box_btns_box);
                        lv_obj_set_style_radius(camera_settings_colorbar_mode_settings_btns[i], LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
                        lv_obj_add_flag(camera_settings_colorbar_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_colorbar_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_colorbar_mode_settings_btns[0], "disable");
                    lv_checkbox_set_text_static(camera_settings_colorbar_mode_settings_btns[1], "enable");
                    lv_checkbox_set_text_static(camera_settings_colorbar_mode_settings_btns[2], "grad ualv");
                }
            }
        }

        {
            camera_settings_night_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_night_mode_setting_box, LV_PCT(60), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_night_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_night_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_night_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_night_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_night_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_night_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_night_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_night_mode_setting_box, 1, LV_STATE_DEFAULT);
            {
                camera_settings_night_mode_setting_box_text_box = lv_obj_create(camera_settings_night_mode_setting_box);
                lv_obj_set_style_width(camera_settings_night_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_night_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_night_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_night_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_night_mode_setting_box_text = lv_label_create(camera_settings_night_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_night_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_night_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_night_mode_setting_box_text, "night mode");
            }

            {
                camera_settings_night_mode_setting_box_btns_box = lv_obj_create(camera_settings_night_mode_setting_box);
                lv_obj_set_style_width(camera_settings_night_mode_setting_box_btns_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_night_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_night_mode_setting_box_btns_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_night_mode_setting_box_btns_box, 4, LV_STATE_DEFAULT);
                lv_obj_set_layout(camera_settings_night_mode_setting_box_btns_box, LV_LAYOUT_FLEX);
                lv_obj_set_flex_flow(camera_settings_night_mode_setting_box_btns_box, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(camera_settings_night_mode_setting_box_btns_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

                {
                    for (int i = 0; i < 1; i++) {
                        camera_settings_night_mode_settings_btns[i] = lv_checkbox_create(camera_settings_night_mode_setting_box_btns_box);
                        lv_obj_add_flag(camera_settings_night_mode_settings_btns[i], LV_OBJ_FLAG_EVENT_BUBBLE);
                        lv_obj_set_user_data(camera_settings_night_mode_settings_btns[i], (void *)(uintptr_t)i);
                    }
                    lv_checkbox_set_text_static(camera_settings_night_mode_settings_btns[0], "Enable");
                }
            }
        }

        {
            camera_settings_brightness_mode_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_brightness_mode_setting_box, LV_PCT(80), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_brightness_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_brightness_mode_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_brightness_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_brightness_mode_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_brightness_mode_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_brightness_mode_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_brightness_mode_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_brightness_mode_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_flex_align(camera_settings_brightness_mode_setting_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            lv_obj_add_flag(camera_settings_brightness_mode_setting_box, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
            {
                camera_settings_brightness_mode_setting_box_text_box = lv_obj_create(camera_settings_brightness_mode_setting_box);
                lv_obj_set_style_width(camera_settings_brightness_mode_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_brightness_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_brightness_mode_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_brightness_mode_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_brightness_mode_setting_box_text = lv_label_create(camera_settings_brightness_mode_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_brightness_mode_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_brightness_mode_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_brightness_mode_setting_box_text, "brightness");
            }

            {
                static const char *opt                         = "-4\n-3\n-2\n-1\n0\n1\n2\n3\n4";
                camera_settings_brightness_mode_setting_roller = lv_roller_create(camera_settings_brightness_mode_setting_box);
                lv_obj_set_style_flex_grow(camera_settings_brightness_mode_setting_roller, 1, LV_STATE_DEFAULT);
                lv_roller_set_visible_row_count(camera_settings_brightness_mode_setting_roller, 4);
                lv_roller_set_options(camera_settings_brightness_mode_setting_roller, opt, LV_ROLLER_MODE_NORMAL);
                lv_obj_set_style_flex_grow(camera_settings_brightness_mode_setting_roller, 4, LV_STATE_DEFAULT);
            }
        }

        {
            camera_settings_saturation_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_saturation_setting_box, LV_PCT(80), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_saturation_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_saturation_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_saturation_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_saturation_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_saturation_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_saturation_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_saturation_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_saturation_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_flex_align(camera_settings_saturation_setting_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            {
                camera_settings_saturation_setting_box_text_box = lv_obj_create(camera_settings_saturation_setting_box);
                lv_obj_set_style_width(camera_settings_saturation_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_saturation_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_saturation_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_saturation_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_saturation_setting_box_text = lv_label_create(camera_settings_saturation_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_saturation_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_saturation_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_saturation_setting_box_text, "saturation");
            }

            {
                static const char *opt                    = "-4\n-3\n-2\n-1\n0\n1\n2\n3\n4";
                camera_settings_saturation_setting_roller = lv_roller_create(camera_settings_saturation_setting_box);
                lv_obj_set_style_flex_grow(camera_settings_saturation_setting_roller, 1, LV_STATE_DEFAULT);
                lv_roller_set_visible_row_count(camera_settings_saturation_setting_roller, 4);
                lv_roller_set_options(camera_settings_saturation_setting_roller, opt, LV_ROLLER_MODE_NORMAL);
                lv_obj_set_style_flex_grow(camera_settings_saturation_setting_roller, 4, LV_STATE_DEFAULT);
            }
        }

        {
            camera_settings_contrast_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_contrast_setting_box, LV_PCT(80), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_contrast_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_contrast_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_contrast_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_contrast_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_contrast_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_contrast_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_contrast_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_contrast_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_flex_align(camera_settings_contrast_setting_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            {
                camera_settings_contrast_setting_box_text_box = lv_obj_create(camera_settings_contrast_setting_box);
                lv_obj_set_style_width(camera_settings_contrast_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_contrast_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_contrast_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_contrast_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_contrast_setting_box_text = lv_label_create(camera_settings_contrast_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_contrast_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_contrast_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_contrast_setting_box_text, "contrast");
            }

            {
                static const char *opt                  = "-4\n-3\n-2\n-1\n0\n1\n2\n3\n4";
                camera_settings_contrast_setting_roller = lv_roller_create(camera_settings_contrast_setting_box);
                lv_obj_set_style_flex_grow(camera_settings_contrast_setting_roller, 1, LV_STATE_DEFAULT);
                lv_roller_set_visible_row_count(camera_settings_contrast_setting_roller, 4);
                lv_roller_set_options(camera_settings_contrast_setting_roller, opt, LV_ROLLER_MODE_NORMAL);
                lv_obj_set_style_flex_grow(camera_settings_contrast_setting_roller, 4, LV_STATE_DEFAULT);
            }
        }

        {
            camera_settings_sharpness_setting_box = lv_obj_create(camera_settings_functions_container);
            lv_obj_set_style_height(camera_settings_sharpness_setting_box, LV_PCT(80), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(camera_settings_sharpness_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_border_side(camera_settings_sharpness_setting_box, LV_BORDER_SIDE_BOTTOM, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(camera_settings_sharpness_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(camera_settings_sharpness_setting_box, lv_color_white(), LV_STATE_DEFAULT);
            // lv_obj_set_style_radius(camera_settings_huedegree_setting_box, 0, LV_STATE_DEFAULT);
            lv_obj_set_layout(camera_settings_sharpness_setting_box, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(camera_settings_sharpness_setting_box, LV_FLEX_FLOW_COLUMN);
            lv_obj_set_style_flex_grow(camera_settings_sharpness_setting_box, 1, LV_STATE_DEFAULT);
            lv_obj_set_flex_align(camera_settings_sharpness_setting_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            {
                camera_settings_sharpness_setting_box_text_box = lv_obj_create(camera_settings_sharpness_setting_box);
                lv_obj_set_style_width(camera_settings_sharpness_setting_box_text_box, LV_PCT(100), LV_STATE_DEFAULT);
                lv_obj_set_style_border_width(camera_settings_sharpness_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_pad_all(camera_settings_sharpness_setting_box_text_box, 0, LV_STATE_DEFAULT);
                lv_obj_set_style_flex_grow(camera_settings_sharpness_setting_box_text_box, 1, LV_STATE_DEFAULT);

                camera_settings_sharpness_setting_box_text = lv_label_create(camera_settings_sharpness_setting_box_text_box);
                lv_obj_set_style_align(camera_settings_sharpness_setting_box_text, LV_ALIGN_CENTER, LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(camera_settings_sharpness_setting_box_text, lv_color_black(), LV_STATE_DEFAULT);
                lv_label_set_text(camera_settings_sharpness_setting_box_text, "HueDegree");
            }

            {
                static const char *opt                   = "-6\n-5\n-4\n-3\n-2\n-1\n0\n1\n2\n3\n4\n5";
                camera_settings_sharpness_setting_roller = lv_roller_create(camera_settings_sharpness_setting_box);
                lv_obj_set_style_flex_grow(camera_settings_sharpness_setting_roller, 1, LV_STATE_DEFAULT);
                lv_roller_set_visible_row_count(camera_settings_sharpness_setting_roller, 4);
                lv_roller_set_options(camera_settings_sharpness_setting_roller, opt, LV_ROLLER_MODE_NORMAL);
                lv_obj_set_style_flex_grow(camera_settings_sharpness_setting_roller, 4, LV_STATE_DEFAULT);
            }
        }
    }

    lv_obj_set_user_data(camera_settings_strobe_setting_box_btns_box, camera_settings_strobe_settings_btns[0]);
    lv_obj_add_state(camera_settings_strobe_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_light_mode_setting_box_btns_box, camera_settings_light_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_light_mode_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_effect_mode_setting_box_btns_box, camera_settings_effect_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_effect_mode_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_zoom_mode_setting_box_btns_box, camera_settings_zoom_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_zoom_mode_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_mirror_flip_mode_setting_box_btns_box, camera_settings_mirror_flip_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_mirror_flip_mode_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_colorbar_mode_setting_box_btns_box, camera_settings_colorbar_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_colorbar_mode_settings_btns[0], LV_STATE_CHECKED);
    lv_obj_set_user_data(camera_settings_night_mode_setting_box_btns_box, camera_settings_night_mode_settings_btns[0]);
    lv_obj_add_state(camera_settings_night_mode_settings_btns[0], LV_STATE_DEFAULT);

    lv_roller_set_selected(camera_settings_brightness_mode_setting_roller, 4, LV_ANIM_OFF);
    lv_roller_set_selected(camera_settings_saturation_setting_roller, 4, LV_ANIM_OFF);
    lv_roller_set_selected(camera_settings_contrast_setting_roller, 4, LV_ANIM_OFF);
    lv_roller_set_selected(camera_settings_sharpness_setting_roller, 6, LV_ANIM_OFF);

    lv_obj_add_event_cb(camera_settings_indicator_return_btn, indicator_return_btn_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_strobe_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_light_mode_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_effect_mode_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_zoom_mode_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_mirror_flip_mode_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_colorbar_mode_setting_box_btns_box, checkboxs_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_night_mode_setting_box_btns_box, single_checkbox_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_brightness_mode_setting_roller, roller_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(camera_settings_saturation_setting_roller, roller_callback, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(camera_settings_contrast_setting_roller, roller_callback, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(camera_settings_sharpness_setting_roller, roller_callback, LV_EVENT_VALUE_CHANGED, nullptr);
}

static bool promise_roller_changed = false;

void        change_to_file_explorer_callback(lv_event_t *e) {
    xSemaphoreGive(camera_exit);
}
void take_photo_callback(lv_event_t *e) {
    xSemaphoreGive(camera_take_photo);
}
void open_setting_callback(lv_event_t *e) {
    lv_screen_load(camera_settings_screen);
}
void indicator_return_btn_callback(lv_event_t *e) {
    if (promise_roller_changed) {
        promise_roller_changed = false;
        xSemaphoreGive(roller_changed);
    }
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
    if (curr_target == camera_settings_strobe_setting_box_btns_box)
        xSemaphoreGive(camera_strobe_setting_changed);
    else if (curr_target == camera_settings_light_mode_setting_box_btns_box)
        xSemaphoreGive(camera_light_mode_changed);
    else if (curr_target == camera_settings_effect_mode_setting_box_btns_box)
        xSemaphoreGive(camera_effect_changed);
    else if (curr_target == camera_settings_zoom_mode_setting_box_btns_box)
        xSemaphoreGive(zoom_mode_changed);
    else if (curr_target == camera_settings_mirror_flip_mode_setting_box_btns_box)
        xSemaphoreGive(mirror_flip_changed);
    else if (curr_target == camera_settings_colorbar_mode_setting_box_btns_box)
        xSemaphoreGive(colorbar_changed);
}

void single_checkbox_callback(lv_event_t *e) {
    lv_obj_t *curr_target = lv_event_get_current_target_obj(e);

    auto      self_data   = (lv_obj_t *)lv_obj_get_user_data(curr_target);
    auto      state       = (uint32_t)lv_obj_get_user_data(self_data);

    lv_obj_set_user_data(self_data, (void *)(state ? 0 : 1));
    xSemaphoreGive(nightmode_changed);
}

void roller_callback(lv_event_t *e) {
    promise_roller_changed = true;
}


void image_eventor_callback(lv_event_t *e) {
    xSemaphoreGive(camera_focus_begin);
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
    set_shot_btn();

    xSemaphoreGive(sema_camera_routine_init_done);

    bool          queue_enable        = false;
    bool          can_catch_scene     = false;
    bool          camera_captured_end = false;
    bool          can_take_photo      = false;
    QueueHandle_t the_queue {};
    uint32_t      resolution {}, format {}, data_length {}, src_w {}, src_h {};
    uint32_t      use_full_buffer     = 1;
    bool          screen_RGB_mode     = true;
    uint32_t      strobe_state        = 0;

    bool          focus_begin         = false;
    bool          already_inited      = false;

    uint32_t      light_mode_state    = 0;
    uint32_t      effect_mode_state   = 0;
    uint32_t      zoom_mode_state     = 0;
    uint32_t      mirror_flip_state   = 0;
    uint32_t      colorbar_mode_state = 0;
    int32_t       brightness_state    = 4;
    uint32_t      nightmode_state     = 0;
    int32_t       satuation           = 0;
    int32_t       contrast            = 0;
    int32_t       sharpness           = 0;

    //
    auto set_settings = [&]()
    {
        OV5640_SetLightMode(&ov5640, light_mode_state ? (1u << (light_mode_state - 1)) : 0);
        OV5640_SetColorEffect(&ov5640, effect_mode_state ? (1u << (effect_mode_state - 1)) : 0);
        // OV5640_ZoomConfig(&ov5640, zoom_mode_state == 0   ? OV5640_ZOOM_x1
        //                            : zoom_mode_state == 1 ? OV5640_ZOOM_x2
        //                            : zoom_mode_state == 2 ? OV5640_ZOOM_x4
        //                            : zoom_mode_state == 4 ? OV5640_ZOOM_x8
        //                                                   : OV5640_ZOOM_x1);
        OV5640_MirrorFlipConfig(&ov5640, mirror_flip_state == OV5640_MIRROR_FLIP_NONE ? OV5640_FLIP
                                         : mirror_flip_state == OV5640_FLIP           ? OV5640_MIRROR_FLIP_NONE
                                         : mirror_flip_state == OV5640_MIRROR         ? OV5640_MIRROR_FLIP
                                         : mirror_flip_state == OV5640_MIRROR_FLIP    ? OV5640_MIRROR
                                                                                      : OV5640_MIRROR_FLIP);
        OV5640_ColorbarModeConfig(&ov5640, colorbar_mode_state);
        OV5640_SetBrightness(&ov5640, (brightness_state - 4));
        OV5640_NightModeConfig(&ov5640, nightmode_state);
        OV5640_SetSaturation(&ov5640, satuation);
        OV5640_SetContrast(&ov5640, contrast);
        OV5640_SetHueDegree(&ov5640, sharpness);
    };

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

            current_resolution = camera_resolution::reso_VGA;
            current_format     = camera_format::format_RGB;
            resolution_parse(resolution, data_length, src_w, src_h, format);

            if (focus_begin) {
                focus_begin = false;
                xTimerStop(camera_single_focus_timer, portMAX_DELAY);
            }

            camera_deinit_have_done = false;
            if (camera_init(can_catch_scene, resolution, format, already_inited) != 0) {
                continue;
            }

            set_settings();
            vTaskDelay(pdMS_TO_TICKS(10));

            queue_enable   = true;
            can_take_photo = true;
            already_inited = true;

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
                if (camera_captured_end)
                    continue;

                camera_captured_end = true;
                if (screen_RGB_mode) {
                    camera_RGB_YCbCr_capture_abort_first_stage_dma(target_dcmi, current_format);
                }
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
                        picture_scaling_fast_preview(jpeg_before_buffer_rgb, capture_screen_buffer, src_w, src_h,
                                                     full_pict_show_size[0], full_pict_show_size[1]);
                        img_dsc.data    = capture_screen_buffer;
                        use_full_buffer = 2;
                    }
                    else {
                        picture_scaling_fast_preview(jpeg_before_buffer_rgb, capture_screen_buffer_2, src_w, src_h,
                                                     full_pict_show_size[0], full_pict_show_size[1]);
                        img_dsc.data    = capture_screen_buffer_2;
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
                    current_resolution = camera_resolution::reso_VGA;
                    current_format     = camera_format::format_RGB;
                    resolution_parse(resolution, data_length, src_w, src_h, format);

                    camera_deinit_have_done = false;
                    if (camera_init(can_catch_scene, resolution, format, true) != 0) {
                        xSemaphoreGive(camera_interface_restart);
                        continue;
                    }

                    vTaskDelay(pdMS_TO_TICKS(100));
                    set_settings();

                    queue_enable        = true;
                    camera_captured_end = false;
                    screen_RGB_mode     = true;
                    can_take_photo      = true;

                    indicator_operate("");
                    camera_start_capture(target_dcmi, current_format,
                                         (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                         (uintptr_t)jpeg_before_buffer_rgb, data_length); // RGB Capture
                }
            }
        }
        else if (the_queue == camera_exit) {
            xSemaphoreTake(camera_exit, 0);
            queue_enable = false;

            if (focus_begin) {
                focus_begin = false;
                xTimerStop(camera_single_focus_timer, portMAX_DELAY);
            }

            camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
            OV5640_Stop(&ov5640);
            screen_image_operate(nullptr);
            vTaskDelay(pdMS_TO_TICKS(20));

            screen_RGB_mode = true;
            can_take_photo  = false;
            send_command_to_main_manage(nullptr, 0, manage_command_type::to_file_explorer, portMAX_DELAY);
        }
        else if (the_queue == camera_error) {
            xSemaphoreTake(camera_error, 0);

            if (focus_begin) {
                focus_begin = false;
                xTimerStop(camera_single_focus_timer, portMAX_DELAY);
            }

            camera_RGB_YCbCr_capture_stop(target_dcmi, current_format);
            camera_deinit(error_message, nullptr);
            already_inited  = false;

            can_catch_scene = false;
            can_take_photo  = false;
        }
        else if (the_queue == camera_take_photo) {
            xSemaphoreTake(camera_take_photo, 0);
            if (!can_take_photo)
                continue;
            can_take_photo = false;

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

            set_settings();

            vTaskDelay(pdMS_TO_TICKS(500));
            queue_enable        = true;
            camera_captured_end = false;
            if (strobe_state == 2) {
                target_dcmi->jpeg_use_strobe = true;
            }
            else {
                target_dcmi->jpeg_use_strobe = false;
            }

            indicator_operate("Please Wait...");

            camera_start_capture(target_dcmi, current_format,
                                 (uintptr_t)(&(D2_SRAM[0])), sizeof(D2_SRAM),
                                 (uintptr_t)jpeg_before_buffer_rgb, 16 * 1024 * 1024); // JPEG Capture
        }
        else if (the_queue == camera_focus_begin) {
            xSemaphoreTake(camera_focus_begin, 0);
            if (!can_take_photo || focus_begin)
                continue;
            can_take_photo = false;
            focus_begin    = true;
            OV5640_Focus_Send_Single(&ov5640);
            indicator_operate("Focus...");
            xTimerStart(camera_single_focus_timer, portMAX_DELAY);
        }
        else if (the_queue == camera_focus_need_restart) {
            xSemaphoreTake(camera_focus_need_restart, 0);
            if (!focus_begin)
                continue;
            xTimerStart(camera_single_focus_timer, portMAX_DELAY);
        }
        else if (the_queue == camera_focus_success) {
            xSemaphoreTake(camera_focus_success, 0);
            can_take_photo = true;
            focus_begin    = false;
            indicator_operate(nullptr);
        }
        else if (the_queue == camera_focus_failed) {
            xSemaphoreTake(camera_focus_failed, 0);
            can_take_photo = true;
            focus_begin    = false;
            indicator_operate(nullptr);
        }
        else if (the_queue == camera_light_mode_changed) {
            xSemaphoreTake(camera_light_mode_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                light_mode_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_light_mode_setting_box_btns_box));
            }
            lv_unlock();
            OV5640_SetLightMode(&ov5640, light_mode_state ? (1u << (light_mode_state - 1)) : 0);
        }
        else if (the_queue == camera_effect_changed) {
            xSemaphoreTake(camera_effect_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                effect_mode_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_effect_mode_setting_box_btns_box));
            }
            lv_unlock();
            OV5640_SetColorEffect(&ov5640, effect_mode_state ? (1u << (effect_mode_state - 1)) : 0);
        }

        else if (the_queue == zoom_mode_changed) {
            xSemaphoreTake(zoom_mode_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                zoom_mode_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_zoom_mode_setting_box_btns_box));
            }
            lv_unlock();
            // OV5640_ZoomConfig(&ov5640, zoom_mode_state == 0   ? OV5640_ZOOM_x1
            //                            : zoom_mode_state == 1 ? OV5640_ZOOM_x2
            //                            : zoom_mode_state == 2 ? OV5640_ZOOM_x4
            //                            : zoom_mode_state == 4 ? OV5640_ZOOM_x8
            //                                                   : OV5640_ZOOM_x1);
        }

        else if (the_queue == mirror_flip_changed) {
            xSemaphoreTake(mirror_flip_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                mirror_flip_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_mirror_flip_mode_setting_box_btns_box));
            }
            lv_unlock();
            OV5640_MirrorFlipConfig(&ov5640, mirror_flip_state == OV5640_MIRROR_FLIP_NONE ? OV5640_FLIP
                                             : mirror_flip_state == OV5640_FLIP           ? OV5640_MIRROR_FLIP_NONE
                                             : mirror_flip_state == OV5640_MIRROR         ? OV5640_MIRROR_FLIP
                                             : mirror_flip_state == OV5640_MIRROR_FLIP    ? OV5640_MIRROR
                                                                                          : OV5640_MIRROR_FLIP);
        }

        else if (the_queue == colorbar_changed) {
            xSemaphoreTake(colorbar_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                colorbar_mode_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_colorbar_mode_setting_box_btns_box));
            }
            lv_unlock();
            OV5640_ColorbarModeConfig(&ov5640, colorbar_mode_state);
        }
        else if (the_queue == nightmode_changed) {
            xSemaphoreTake(nightmode_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                nightmode_state = (uint32_t)lv_obj_get_user_data((lv_obj_t *)lv_obj_get_user_data(camera_settings_night_mode_setting_box_btns_box));
            }
            lv_unlock();
            OV5640_NightModeConfig(&ov5640, nightmode_state);
        }
        else if (the_queue == roller_changed) {
            xSemaphoreTake(roller_changed, 0);
            if (!can_take_photo)
                continue;

            lv_lock();
            {
                satuation        = (int32_t)lv_roller_get_selected(camera_settings_saturation_setting_roller) - 4;
                contrast         = (int32_t)lv_roller_get_selected(camera_settings_contrast_setting_roller) - 3;
                sharpness        = (int32_t)lv_roller_get_selected(camera_settings_sharpness_setting_roller) - 6;
                brightness_state = (int32_t)lv_roller_get_selected(camera_settings_brightness_mode_setting_roller) - 4;
            }
            lv_unlock();
            OV5640_SetSaturation(&ov5640, satuation);
            OV5640_SetContrast(&ov5640, contrast);
            OV5640_SetHueDegree(&ov5640, sharpness);
            OV5640_SetBrightness(&ov5640, brightness_state);
        }
    }
}

extern "C" void EXTI2_IRQHandler() {
    HAL_GPIO_EXTI_IRQHandler(SHOT_Pin);
}

static void shot_btn_callback() {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(camera_take_photo, &woken);
    portYIELD_FROM_ISR(woken);
}

static void set_shot_btn() {
#ifndef ALINTEK_BOARD
    shot_btn_isr_callback            = shot_btn_callback;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = SHOT_Pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SHOT_Port, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI2_IRQn);
#endif
}

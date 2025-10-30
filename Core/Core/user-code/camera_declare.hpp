#pragma once

#include "prj_header.hpp"

enum class camera_resolution {
    reso_5M,
    reso_QXGA,
    reso_1080p,
    reso_UXGA,
    reso_SXGA,
    reso_WXGA_plus,
    reso_WXGA,
    reso_XGA,
    reso_SVGA,
    reso_WVGA,
    reso_VGA, // 640*480
    reso_PSP,
    reso_QVGA,
    reso_QQVGA
};

enum class camera_format {
    format_RGB,
    format_YCbCr,
    format_JPEG
};

enum class message_process : uint32_t {
    ERROR_STOP  = 1 << 0,
    CANNOT_GET  = 1 << 1,
    DMA_ABORT   = 1 << 2,
    MDMA_ABORT  = 1 << 3,
    OK          = 1 << 4,
    CAPTURE_END = 1 << 5,
    DATA_END    = 1 << 6
};

struct Camera_DCMI_HandleType;

struct Camera_DCMI_Data {
    DCMI_HandleTypeDef      instance;
    DMA_HandleTypeDef       first_stage_dma;
    MDMA_HandleTypeDef      second_stage_dma;

    Camera_DCMI_HandleType *parent;
    //
    uintptr_t double_buffer_first;
    uintptr_t double_buffer_second;
    uintptr_t final_buffer;
    size_t    final_buffer_len;
    size_t    half_middle_buffer_words;
    size_t    double_buffer_transmit_count;
    size_t    per_block_word;
    size_t    block_count;

    size_t    jpeg_data_count_calculate;
    bool      jpeg_sw_final;

    bool      single_buffer_fine;
    bool      dma_not_full;
    bool      jpeg_mode;

    bool      jpeg_use_strobe;
    bool      always_use_strobe;
    //
    bool               mdma_first_buffer;
    EventGroupHandle_t eg;
    QueueHandle_t      MDMA_sync;
};

struct Camera_DCMI_HandleType : public Camera_DCMI_Data {
    std::vector<MDMA_LinkNodeTypeDef> the_node_list;
};

extern OV5640_IO_t             ov5640_io;
extern OV5640_Object_t         ov5640;
extern soft_sccb_handle        my_sccb;
extern Camera_DCMI_HandleType *target_dcmi;

extern camera_resolution       current_resolution;
extern camera_format           current_format;
extern bool                    PCF8574_init;
extern bool                    camera_deinit_have_done;
extern bool                    target_dcmi_is_ok;

extern Camera_DCMI_HandleType  RGB_hdcmi;   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
extern Camera_DCMI_HandleType  YCbCr_hdcmi; // YCbCr(4:2:2) => Encode Inside(JPEG)
extern Camera_DCMI_HandleType  JPEG_hdcmi;  // JPEG(4:2:2)
extern TimerHandle_t           camera_single_focus_timer;
extern TimerHandle_t           camera_constant_focus_timer;


extern SemaphoreHandle_t       camera_new_message;
extern SemaphoreHandle_t       camera_exit;
extern SemaphoreHandle_t       camera_error;
extern SemaphoreHandle_t       camera_take_photo;
extern QueueSetHandle_t        camera_queue_set;
extern SemaphoreHandle_t       camera_strobe_setting_changed;
extern SemaphoreHandle_t       camera_light_mode_changed;
extern SemaphoreHandle_t       camera_effect_changed;
extern SemaphoreHandle_t       camera_focus_success;
extern SemaphoreHandle_t       camera_focus_failed;
extern SemaphoreHandle_t       camera_focus_need_restart;
extern SemaphoreHandle_t       camera_focus_begin;
extern SemaphoreHandle_t       zoom_mode_changed;
extern SemaphoreHandle_t       mirror_flip_changed;
extern SemaphoreHandle_t       colorbar_changed;
extern SemaphoreHandle_t       nightmode_changed;
extern SemaphoreHandle_t       roller_changed;

extern lv_obj_t               *screen_container;
extern lv_obj_t               *camera_capture_image;

extern lv_obj_t               *button_container;
extern lv_obj_t               *take_photo_button;
extern lv_obj_t               *change_to_file_explorer_button;
extern lv_obj_t               *open_setting_button;
extern lv_obj_t               *take_photo_label;
extern lv_obj_t               *change_to_file_explorer_label;
extern lv_obj_t               *open_setting_label;

extern lv_obj_t               *indicator_label;

//
extern uint8_t D2_SRAM[128 * 1024] IN_SRAM2 __ALIGNED(32);

//
void              dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void              dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void              dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void              dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle);
void              dcmi_data_structure_init();
void              dcmi_io_deinit_ov5640();
int32_t           ov5640_init();
int32_t           ov5640_deinit();
void              OV5640_PWDN_Set(uint8_t sta);
int32_t           ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length);
int32_t           ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length);

void              lvgl_create_camera_interface();
void              dcmi_capture_resource_init();
int               camera_init(bool &can_catch_scene, uint32_t resolution, uint32_t format, bool just_change);
void              camera_deinit(const char *error_message, void *error_picture);

void              camera_RGB_YCbCr_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma);
void              camera_RGB_YCbCr_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma);
void              camera_RGB_YCbCr_DMA_Error_Cb(DMA_HandleTypeDef *hdma);
void              camera_RGB_YCbCr_DMA_Abort_Cb(DMA_HandleTypeDef *hdma);
void              camera_RGB_YCbCr_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_RGB_YCbCr_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_RGB_YCbCr_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_RGB_YCbCr_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma);

void              camera_JPEG_DMA_Cplt_Cb(DMA_HandleTypeDef *hdma);
void              camera_JPEG_DMA_M1_Cplt_Cb(DMA_HandleTypeDef *hdma);
void              camera_JPEG_DMA_Error_Cb(DMA_HandleTypeDef *hdma);
void              camera_JPEG_DMA_Abort_Cb(DMA_HandleTypeDef *hdma);
void              camera_JPEG_MDMA_RepeatBlock_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_JPEG_MDMA_Cplt_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_JPEG_MDMA_Error_Cb(MDMA_HandleTypeDef *hmdma);
void              camera_JPEG_MDMA_Abort_Cb(MDMA_HandleTypeDef *hmdma);

void              change_to_file_explorer_callback(lv_event_t *e);
void              take_photo_callback(lv_event_t *e);
void              open_setting_callback(lv_event_t *e);
void              indicator_return_btn_callback(lv_event_t *e);
void              checkboxs_callback(lv_event_t *e);
void              single_checkbox_callback(lv_event_t *e);
void              roller_callback(lv_event_t *e);
void              image_eventor_callback(lv_event_t *e);

void              indicator_operate(const char *message);
void              screen_image_operate(void *source);
void              calculate_decompose(size_t &x, size_t &y, size_t &z, size_t y_max);

void              resolution_parse(uint32_t &resolution, uint32_t &data_length, uint32_t &src_w, uint32_t &src_h, uint32_t &format);
uint32_t          camera_capture_process(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
HAL_StatusTypeDef camera_start_capture(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format,
                                       uintptr_t middle_buffer, size_t middle_buffer_len,
                                       uintptr_t final_buffer, size_t final_buffer_len);
HAL_StatusTypeDef camera_capture_resume(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
void              camera_RGB_YCbCr_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
void              camera_RGB_YCbCr_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);

void              camera_JPEG_capture_abort_first_stage_dma(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
void              camera_JPEG_capture_stop(Camera_DCMI_HandleType *Camera_DCMI, camera_format target_format);
void              MY_HAL_DCMI_IRQHandler(DCMI_HandleTypeDef *hdcmi);

void              camera_single_focus_timer_callback(TimerHandle_t xTimer);
void              camera_constant_focus_timer_callback(TimerHandle_t xTimer);

LV_FONT_DECLARE(photo_folder_setting)
#define OV5640_ADDR                  0x78
#define OV5640_RST(n)                (HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define OV5640_STROBE(n)             (HAL_GPIO_WritePin(DCMI_XCLK_GPIO_Port, DCMI_XCLK_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define DCMI_PWDN_IO                 2
#define MY_TAKE_PHOTO_SYMBOL         "\xE2\x97\x89"

// #define ALINTEK_BOARD

#define FIRST_STAGE_DMA_ERROR        0b000000000001u
#define FIRST_STAGE_DMA_ABORT        0b000000000010u
#define FIRST_STAGE_DMA_CPLT         0b000000000100u
#define FIRST_STAGE_DMA_M1_CPLT      0b000000001000u
#define FIRST_STAGE_DMA_MASK         0b000000001111u

#define SECOND_STAGE_DMA_ERROR       0b000000010000u
#define SECOND_STAGE_DMA_ABORT       0b000000100000u
#define SECOND_STAGE_DMA_CPLT        0b000001000000u
#define SECOND_STAGE_DMA_REPEAT_CPLT 0b000010000000u
#define SECOND_STAGE_DMA_MASK        0b000011110000u

#define CAMERA_CAPTURE_OK            0b000100000000u
#define CAMERA_CAPTURE_ERROR         0b001000000000u
#define CAMERA_CAPTURE_MASK          0b001100000000u

#define CAMERA_CAPTURE_ALL_MASK      ((FIRST_STAGE_DMA_MASK) | (SECOND_STAGE_DMA_MASK) | (CAMERA_CAPTURE_MASK))

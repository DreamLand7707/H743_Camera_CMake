#define FILE_DEBUG 1

#include "prj_header.hpp"

#define ALINTEK_BOARD

static void     lvgl_create_camera_interface();
static void     dcmi_data_structure_init();
static void     dcmi_resource_init();

static void     change_to_file_explorer_callback(lv_event_t *e);
static void     take_photo_callback(lv_event_t *e);
static void     open_setting_callback(lv_event_t *e);

static int32_t  ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length);
static int32_t  ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length);
static int32_t  ov5640_init();
static int32_t  ov5640_deinit();
static void     OV5640_PWDN_Set(uint8_t sta);

static void     dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle);
static void     dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle);
static void     dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle);
static void     dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle);
static void     dcmi_io_deinit_ov5640();

extern "C" void DMA1_Stream0_IRQHandler(void);
extern "C" void DCMI_IRQHandler(void);

LV_FONT_DECLARE(photo_folder_setting)

#define MY_TAKE_PHOTO_SYMBOL "\xE2\x97\x89"
#define MY_FOLDER_SYMBOL     "\xF0\x9F\x93\x81"
#define MY_SETTING_SYMBOL    "\xE2\x9A\x99"
#define OV5640_ADDR          0x78
#define OV5640_RST(n)        (HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define DCMI_PWDN_IO         2

DCMI_HandleTypeDef RGB_hdcmi;   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
DCMI_HandleTypeDef YCbCr_hdcmi; // YCbCr(4:2:2) => Encode Inside(JPEG)
DCMI_HandleTypeDef JPEG_hdcmi;  // JPEG(4:2:2)

SemaphoreHandle_t  camera_interface_changed;

namespace
{
    OV5640_IO_t       ov5640_io {};
    OV5640_Object_t   ov5640 {};
    soft_sccb_handle  my_sccb;
    DMA_HandleTypeDef my_hdma_dcmi;

    enum class camera_resolution {
        reso_5000k,
        reso_1080p,
        reso_vga, // 640*480
    };

    enum class camera_format {
        format_RGB,
        format_YCbCr,
        format_JPEG
    };

    camera_resolution   current_resolution;
    camera_format       current_format;

    SemaphoreHandle_t   camera_new_scene;
    SemaphoreHandle_t   camera_exit;

    QueueSetHandle_t    camera_queue_set;
    DCMI_HandleTypeDef *target_dcmi;
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

    QueueHandle_t the_queue;

    while (1) {
        xSemaphoreTake(camera_interface_changed, portMAX_DELAY);
        uint32_t resolution, format;
        uint32_t data_length;
        uint32_t src_w, src_h;
        bool     can_catch_scene;
        switch (current_resolution) {
        case camera_resolution::reso_5000k: {
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
        lv_lock();
        {
            lv_obj_remove_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, "Initialize DCMI Interface...");
        }
        lv_unlock();
        if (HAL_DCMI_Init(target_dcmi) != HAL_OK) {
            lv_lock();
            {
                lv_label_set_text_static(indicator_label, "Initialize DCMI Interface Failed!");
            }
            lv_unlock();
            continue;
        }

        lv_lock();
        {
            lv_obj_remove_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, "Initialize Camera...");
        }
        lv_unlock();

        (void)OV5640_RegisterBusIO(&ov5640, &ov5640_io);
        if (OV5640_Init(&ov5640, resolution, format) != OV5640_OK) {
            lv_lock();
            {
                lv_label_set_text_static(indicator_label, "Initialize Camera Failed!");
            }
            lv_unlock();
            continue;
        }

        lv_lock();
        {
            lv_obj_remove_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, "Start Camera...");
        }
        lv_unlock();
        OV5640_Start(&ov5640);

        lv_lock();
        {
            lv_obj_add_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(indicator_label, "");
        }
        lv_unlock();

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

                lv_lock();
                {
                    lv_image_set_src(camera_capture_image, &img_dsc);
                    lv_image_set_align(camera_capture_image, LV_IMAGE_ALIGN_CENTER);
                }
                lv_unlock();

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
                lv_lock();
                {
                    lv_label_set_text_static(indicator_label, "");
                    lv_obj_add_flag(indicator_label, LV_OBJ_FLAG_HIDDEN);
                    lv_image_set_src(camera_capture_image, nullptr);
                }
                lv_unlock();
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


static void dcmi_data_structure_init() {
    RGB_hdcmi.Instance                = DCMI;
    RGB_hdcmi.Init.SynchroMode        = DCMI_SYNCHRO_HARDWARE;
    RGB_hdcmi.Init.PCKPolarity        = DCMI_PCKPOLARITY_RISING;
    RGB_hdcmi.Init.VSPolarity         = DCMI_VSPOLARITY_HIGH;
    RGB_hdcmi.Init.HSPolarity         = DCMI_HSPOLARITY_HIGH;
    RGB_hdcmi.Init.CaptureRate        = DCMI_CR_ALL_FRAME;
    RGB_hdcmi.Init.ExtendedDataMode   = DCMI_EXTEND_DATA_8B;
    RGB_hdcmi.Init.JPEGMode           = DCMI_JPEG_DISABLE;
    RGB_hdcmi.Init.ByteSelectMode     = DCMI_BSM_ALL;
    RGB_hdcmi.Init.ByteSelectStart    = DCMI_OEBS_ODD;
    RGB_hdcmi.Init.LineSelectMode     = DCMI_LSM_ALL;
    RGB_hdcmi.Init.LineSelectStart    = DCMI_OELS_ODD;
    RGB_hdcmi.MspInitCallback         = dcmi_rgb_mspinit;
    RGB_hdcmi.MspDeInitCallback       = dcmi_msp_deinit;

    YCbCr_hdcmi.Instance              = DCMI;
    YCbCr_hdcmi.Init.SynchroMode      = DCMI_SYNCHRO_HARDWARE;
    YCbCr_hdcmi.Init.PCKPolarity      = DCMI_PCKPOLARITY_RISING;
    YCbCr_hdcmi.Init.VSPolarity       = DCMI_VSPOLARITY_HIGH;
    YCbCr_hdcmi.Init.HSPolarity       = DCMI_HSPOLARITY_HIGH;
    YCbCr_hdcmi.Init.CaptureRate      = DCMI_CR_ALL_FRAME;
    YCbCr_hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    YCbCr_hdcmi.Init.JPEGMode         = DCMI_JPEG_DISABLE;
    YCbCr_hdcmi.Init.ByteSelectMode   = DCMI_BSM_ALL;
    YCbCr_hdcmi.Init.ByteSelectStart  = DCMI_OEBS_ODD;
    YCbCr_hdcmi.Init.LineSelectMode   = DCMI_LSM_ALL;
    YCbCr_hdcmi.Init.LineSelectStart  = DCMI_OELS_ODD;
    YCbCr_hdcmi.MspInitCallback       = dcmi_ycbcr_mspinit;
    YCbCr_hdcmi.MspDeInitCallback     = dcmi_msp_deinit;

    JPEG_hdcmi.Instance               = DCMI;
    JPEG_hdcmi.Init.SynchroMode       = DCMI_SYNCHRO_HARDWARE;
    JPEG_hdcmi.Init.PCKPolarity       = DCMI_PCKPOLARITY_RISING;
    JPEG_hdcmi.Init.VSPolarity        = DCMI_VSPOLARITY_HIGH;
    JPEG_hdcmi.Init.HSPolarity        = DCMI_HSPOLARITY_HIGH;
    JPEG_hdcmi.Init.CaptureRate       = DCMI_CR_ALL_FRAME;
    JPEG_hdcmi.Init.ExtendedDataMode  = DCMI_EXTEND_DATA_8B;
    JPEG_hdcmi.Init.JPEGMode          = DCMI_JPEG_ENABLE;
    JPEG_hdcmi.Init.ByteSelectMode    = DCMI_BSM_ALL;
    JPEG_hdcmi.Init.ByteSelectStart   = DCMI_OEBS_ODD;
    JPEG_hdcmi.Init.LineSelectMode    = DCMI_LSM_ALL;
    JPEG_hdcmi.Init.LineSelectStart   = DCMI_OELS_ODD;
    JPEG_hdcmi.MspInitCallback        = dcmi_jpeg_mspinit;
    JPEG_hdcmi.MspDeInitCallback      = dcmi_msp_deinit;

    static auto delay_handle          = [](uint32_t delay)
    {
        uint32_t i;
        while (delay--) {
            i = 50;
            while (i--)
                ;
        }
    };
    my_sccb.address_withwr          = OV5640_ADDR;
    my_sccb.parameters.delay_handle = delay_handle;
    my_sccb.parameters.frequency    = 200000;
    my_sccb.port_setting.SCL_port   = DCMI_SCL_GPIO_Port;
    my_sccb.port_setting.SCL_pin    = DCMI_SCL_Pin;
    my_sccb.port_setting.SDA_port   = DCMI_SDA_GPIO_Port;
    my_sccb.port_setting.SDA_pin    = DCMI_SDA_Pin;

    static auto get_tick            = []()
    {
        return (int32_t)(HAL_GetTick());
    };
    ov5640_io.Address  = OV5640_ADDR;
    ov5640_io.GetTick  = get_tick;
    ov5640_io.Init     = ov5640_init;
    ov5640_io.DeInit   = ov5640_deinit;
    ov5640_io.ReadReg  = ov5640_read_series_reg;
    ov5640_io.WriteReg = ov5640_write_series_reg;

    current_resolution = camera_resolution::reso_vga;
    current_format     = camera_format::format_RGB;
}

static void dcmi_io_deinit_ov5640() {
#ifdef ALINTEK_BOARD
    static bool PCF8574_init = false;
    if (!PCF8574_init) {
        PCF8574_Init();
        PCF8574_init = true;
    }
#endif
    OV5640_PWDN_Set(1);
    OV5640_RST(0);
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

static void    take_photo_callback(lv_event_t *e) {}
static void    open_setting_callback(lv_event_t *e) {}

static int32_t ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_RD_Reg(&my_sccb, reg + idx, pdata + idx))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

static int32_t ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_WR_Reg(&my_sccb, reg + idx, data[idx]))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

static void OV5640_PWDN_Set(uint8_t sta) {
#ifdef ALINTEK_BOARD
    PCF8574_WriteBit(DCMI_PWDN_IO, sta);
#else
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, sta ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

static int32_t ov5640_init() {
    uint8_t  reg1, reg2;
    uint16_t reg;

    OV5640_RST(0);
    timer_delay_ms(20);
    OV5640_PWDN_Set(0);
    timer_delay_ms(5);
    OV5640_RST(1);
    timer_delay_ms(20);

    timer_delay_ms(5);
    if (OV5640_RD_Reg(&my_sccb, OV5640_CHIP_ID_HIGH_BYTE, &reg1))
        return OV5640_ERROR;
    if (OV5640_RD_Reg(&my_sccb, OV5640_CHIP_ID_LOW_BYTE, &reg2))
        return OV5640_ERROR;
    reg = reg1;
    reg <<= 8u;
    reg |= reg2;
    if (reg != OV5640_ID) {
        return OV5640_ERROR;
    }
    if (OV5640_WR_Reg(&my_sccb, 0x3103, 0X11))
        return OV5640_ERROR;
    if (OV5640_WR_Reg(&my_sccb, 0X3008, 0X82))
        return OV5640_ERROR;
    timer_delay_ms(10);

    return OV5640_OK;
}

static int32_t ov5640_deinit() { return OV5640_OK; }


//

static void dcmi_clk_pin_init() {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_DCMI_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_8;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin       = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_I2CEx_EnableFastModePlus(SYSCFG_PMCR_I2C_PB7_FMP);

    HAL_I2CEx_EnableFastModePlus(SYSCFG_PMCR_I2C_PB8_FMP);

    HAL_I2CEx_EnableFastModePlus(SYSCFG_PMCR_I2C_PB9_FMP);
}

static void dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    my_hdma_dcmi.Instance                 = DMA1_Stream0;
    my_hdma_dcmi.Init.Request             = DMA_REQUEST_DCMI;
    my_hdma_dcmi.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    my_hdma_dcmi.Init.PeriphInc           = DMA_PINC_DISABLE;
    my_hdma_dcmi.Init.MemInc              = DMA_MINC_ENABLE;
    my_hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    my_hdma_dcmi.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    my_hdma_dcmi.Init.Mode                = DMA_CIRCULAR;
    my_hdma_dcmi.Init.Priority            = DMA_PRIORITY_HIGH;
    my_hdma_dcmi.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    my_hdma_dcmi.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    my_hdma_dcmi.Init.MemBurst            = DMA_MBURST_INC4;
    my_hdma_dcmi.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&my_hdma_dcmi) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, my_hdma_dcmi);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
}

static void dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    my_hdma_dcmi.Instance                 = DMA1_Stream0;
    my_hdma_dcmi.Init.Request             = DMA_REQUEST_DCMI;
    my_hdma_dcmi.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    my_hdma_dcmi.Init.PeriphInc           = DMA_PINC_DISABLE;
    my_hdma_dcmi.Init.MemInc              = DMA_MINC_ENABLE;
    my_hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    my_hdma_dcmi.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    my_hdma_dcmi.Init.Mode                = DMA_CIRCULAR;
    my_hdma_dcmi.Init.Priority            = DMA_PRIORITY_HIGH;
    my_hdma_dcmi.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    my_hdma_dcmi.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    my_hdma_dcmi.Init.MemBurst            = DMA_MBURST_INC4;
    my_hdma_dcmi.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&my_hdma_dcmi) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, my_hdma_dcmi);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
}

static void dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle) {
    dcmi_clk_pin_init();

    my_hdma_dcmi.Instance                 = DMA1_Stream0;
    my_hdma_dcmi.Init.Request             = DMA_REQUEST_DCMI;
    my_hdma_dcmi.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    my_hdma_dcmi.Init.PeriphInc           = DMA_PINC_DISABLE;
    my_hdma_dcmi.Init.MemInc              = DMA_MINC_ENABLE;
    my_hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    my_hdma_dcmi.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    my_hdma_dcmi.Init.Mode                = DMA_CIRCULAR;
    my_hdma_dcmi.Init.Priority            = DMA_PRIORITY_HIGH;
    my_hdma_dcmi.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    my_hdma_dcmi.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    my_hdma_dcmi.Init.MemBurst            = DMA_MBURST_INC4;
    my_hdma_dcmi.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&my_hdma_dcmi) != HAL_OK) {
        Error_Handler();
    }

    __HAL_LINKDMA(dcmiHandle, DMA_Handle, my_hdma_dcmi);

    HAL_NVIC_SetPriority(DCMI_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
}



static void dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle) {
    __HAL_RCC_DCMI_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_8);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_11);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);

    HAL_DMA_DeInit(dcmiHandle->DMA_Handle);
    HAL_NVIC_DisableIRQ(DCMI_IRQn);
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    BaseType_t should_yield = pdFALSE;
    xSemaphoreGiveFromISR(camera_new_scene, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi) {
    // debug("[HAL_DCMI_ErrorCallback] ", hdcmi->ErrorCode);
    // debug("DCMI ErrorCode: %u;", hdcmi->ErrorCode);
    // debug("DMA ErrorCode: %u\n", hdcmi->DMA_Handle->ErrorCode);
}

void DMA1_Stream0_IRQHandler(void) {
    HAL_DMA_IRQHandler(&my_hdma_dcmi);
}

void DCMI_IRQHandler(void) {
    HAL_DCMI_IRQHandler(target_dcmi);
}

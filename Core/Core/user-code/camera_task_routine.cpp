
#include "prj_header.hpp"

#define ALINTEK_BOARD

static void    lvgl_create_camera_interface();
static void    dcmi_data_structure_init();

static void    change_to_file_explorer_callback(lv_event_t *e);
static void    take_photo_callback(lv_event_t *e);
static void    open_setting_callback(lv_event_t *e);

static int32_t ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length);
static int32_t ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length);
static int32_t ov5640_init();
static int32_t ov5640_deinit();

LV_FONT_DECLARE(photo_folder_setting)

#define MY_TAKE_PHOTO_SYMBOL "\xE2\x97\x89"
#define MY_FOLDER_SYMBOL     "\xF0\x9F\x93\x81"
#define MY_SETTING_SYMBOL    "\xE2\x9A\x99"
#define OV5640_ADDR          0x78
#define OV5640_RST(n) (HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define DCMI_PWDN_IO  2

namespace
{
    OV5640_IO_t      ov5640_io {};
    OV5640_Object_t  ov5640 {};
    soft_sccb_handle my_sccb;
} // namespace

void camera_task_routine(void const *argument) {
    xSemaphoreTake(sema_camera_routine_start, portMAX_DELAY);

    dcmi_data_structure_init();
    lvgl_create_camera_interface();

    xSemaphoreGive(sema_camera_routine_init_done);
    while (1) {
        vTaskSuspend(xTaskGetCurrentTaskHandle());
    }
}

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

} // namespace

static void dcmi_data_structure_init() {
    hdcmi.Instance              = DCMI;
    hdcmi.Init.SynchroMode      = DCMI_SYNCHRO_HARDWARE;
    hdcmi.Init.PCKPolarity      = DCMI_PCKPOLARITY_FALLING;
    hdcmi.Init.VSPolarity       = DCMI_VSPOLARITY_HIGH;
    hdcmi.Init.HSPolarity       = DCMI_HSPOLARITY_HIGH;
    hdcmi.Init.CaptureRate      = DCMI_CR_ALL_FRAME;
    hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    hdcmi.Init.JPEGMode         = DCMI_JPEG_DISABLE;
    hdcmi.Init.ByteSelectMode   = DCMI_BSM_ALL;
    hdcmi.Init.ByteSelectStart  = DCMI_OEBS_ODD;
    hdcmi.Init.LineSelectMode   = DCMI_LSM_ALL;
    hdcmi.Init.LineSelectStart  = DCMI_OELS_ODD;

    static auto delay_handle    = [](uint32_t delay)
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
    
    OV5640_RegisterBusIO(&ov5640, &ov5640_io);
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
}

static void change_to_file_explorer_callback(lv_event_t *e) {
    if (send_command_to_main_manage(nullptr, 0, manage_command_type::to_file_explorer, 0) != pdPASS) {
        jprintf(0, "Can't Change to file explorer");
    }
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
#ifdef ALINTEK_BOARD
    PCF8574_Init();
#endif
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

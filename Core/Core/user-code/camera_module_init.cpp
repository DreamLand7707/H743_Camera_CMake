
#include "camera_declare.hpp"

bool PCF8574_init = false;

int  camera_init(bool &can_catch_scene, uint32_t resolution, uint32_t format) {
    // DCMI Init -> IO Init -> OV5640 Init -> OV5640 Start -> DCMI DMA

    indicator_operate("Initialize DCMI Interface...");
    if (HAL_DCMI_Init(&(target_dcmi->data.instance)) != HAL_OK) {
        indicator_operate("Initialize DCMI Interface Failed!");
        can_catch_scene = false;
        return -1;
    }

    indicator_operate("Initialize Camera...");
    (void)OV5640_RegisterBusIO(&ov5640, &ov5640_io);
    if (OV5640_Init_General_Mode(&ov5640, resolution, format) != OV5640_OK) {
        indicator_operate("Initialize Camera Failed!");
        can_catch_scene = false;
        return -1;
    }

    indicator_operate("Start Camera...");
    OV5640_Start(&ov5640);

    indicator_operate(nullptr);

    return 0;
}

void camera_deinit(const char *error_message, void *error_picture) {
    if (!camera_deinit_have_done) {
        indicator_operate(error_message);
        screen_image_operate(error_picture);

        OV5640_Stop(&ov5640);
        OV5640_DeInit(&ov5640);
        dcmi_io_deinit_ov5640();
        HAL_DCMI_DeInit(&(target_dcmi->data.instance));
        camera_deinit_have_done = true;
    }
}

int32_t ov5640_init() {
    uint8_t  reg1, reg2;
    uint16_t reg;

#ifdef ALINTEK_BOARD
    if (!PCF8574_init) {
        PCF8574_Init();
        PCF8574_init = true;
    }
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

    return OV5640_OK;
}

int32_t ov5640_deinit() { return OV5640_OK; }



int32_t ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_RD_Reg(&my_sccb, reg + idx, pdata + idx))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

int32_t ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length) {
    (void)address;
    for (uint16_t idx = 0; idx < length; idx++) {
        if (OV5640_WR_Reg(&my_sccb, reg + idx, data[idx]))
            return OV5640_ERROR;
    }
    return OV5640_OK;
}

void OV5640_PWDN_Set(uint8_t sta) {
#ifdef ALINTEK_BOARD
    PCF8574_WriteBit(DCMI_PWDN_IO, sta);
#else
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, sta ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
}

void dcmi_io_deinit_ov5640() {
#ifdef ALINTEK_BOARD
    if (!PCF8574_init) {
        PCF8574_Init();
        PCF8574_init = true;
    }
#endif
    OV5640_PWDN_Set(1);
    OV5640_RST(0);
}

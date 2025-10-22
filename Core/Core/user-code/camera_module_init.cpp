
#include "camera_declare.hpp"

bool PCF8574_init = false;

int  camera_init(bool &can_catch_scene, framesize_t resolution, pixformat_t format, bool just_change) {
    // DCMI Init -> IO Init -> OV5640 Init -> OV5640 Start -> DCMI DMA
    target_dcmi->parent = target_dcmi;
    if (HAL_DCMI_Init(&(target_dcmi->instance)) != HAL_OK) {
        if (!just_change) {
            indicator_operate("Initialize DCMI Interface Failed!");
        }
        can_catch_scene = false;
        return -1;
    }

    if (!just_change) {
        indicator_operate("Initialize DCMI Interface...");
        if (HAL_DCMI_Init(&(target_dcmi->instance)) != HAL_OK) {
            indicator_operate("Initialize DCMI Interface Failed!");
            can_catch_scene = false;
            return -1;
        }
        indicator_operate("Initialize Camera...");

        ov5640_low_level_init();

        if (ov5640_sensor.reset(&ov5640_sensor) != 0) {
            indicator_operate("Initialize Camera Failed!");
            can_catch_scene = false;
            return -1;
        }

        ov5640_sensor.status.framesize = resolution;
        ov5640_sensor.pixformat        = format;

        if (ov5640_sensor.set_framesize(&ov5640_sensor, resolution) != 0) {
            indicator_operate("Initialize Camera Failed!");
            can_catch_scene = false;
            return -1;
        }

        if (ov5640_sensor.set_pixformat(&ov5640_sensor, format) != 0) {
            indicator_operate("Initialize Camera Failed!");
            can_catch_scene = false;
            return -1;
        }

        if (ov5640_sensor.init_status(&ov5640_sensor) != 0) {
            indicator_operate("Initialize Camera Failed!");
            can_catch_scene = false;
            return -1;
        }

        indicator_operate(nullptr);
    }
    else {
        HAL_DCMI_DeInit(&(target_dcmi->instance));
        if (HAL_DCMI_Init(&(target_dcmi->instance)) != HAL_OK) {
            can_catch_scene = false;
            return -1;
        }
        if (ov5640_sensor.set_pixformat(&ov5640_sensor, format) != 0) {
            return -1;
        }
        if (ov5640_sensor.set_framesize(&ov5640_sensor, resolution) != 0) {
            return -1;
        }
    }

    can_catch_scene = true;
    return 0;
}

void camera_deinit(const char *error_message, void *error_picture) {
    if (!camera_deinit_have_done) {
        indicator_operate(error_message);
        screen_image_operate(error_picture);

        dcmi_io_deinit_ov5640();
        HAL_DCMI_DeInit(&(target_dcmi->instance));
        camera_deinit_have_done = true;
    }
}

int32_t ov5640_low_level_init() {
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

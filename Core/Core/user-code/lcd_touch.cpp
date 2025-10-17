/*
 * lcd_touch.cpp
 *
 *  Created on: Nov 19, 2024
 *      Author: DrL潇湘
 */

#include "lcd_touch.hpp"

soft_iic_port_origin scr_touch_iic_port;
soft_iic_param       scr_touch_iic_param;
soft_iic_handle      scr_touch_iic_handle;
touch_ic_type        touch_ic;

const uint8_t        Config_table[] =
    {0x80, 0x47,
     0x41, 0xE0, 0x01, 0x10, 0x01, 0x05, 0x0F, 0x00, 0x02, 0x08,
     0x28, 0x05, 0x50, 0x32, 0x03, 0x00, 0x00, 0x00, 0xFF, 0xFF,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x89, 0x28, 0x0A,
     0x17, 0x15, 0x31, 0x0D, 0x00, 0x00, 0x02, 0x9B, 0x03, 0x25,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
     0x00, 0x0F, 0x94, 0x94, 0xC5, 0x02, 0x07, 0x00, 0x00, 0x04,
     0x8D, 0x13, 0x00, 0x5C, 0x1E, 0x00, 0x3C, 0x30, 0x00, 0x29,
     0x4C, 0x00, 0x1E, 0x78, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x12, 0x14, 0x16,
     0x18, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0x00, 0x02, 0x04, 0x05, 0x06, 0x08, 0x0A, 0x0C,
     0x0E, 0x1D, 0x1E, 0x1F, 0x20, 0x22, 0x24, 0x28, 0x29, 0xFF,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t Send_Cfg(uint8_t mode) {
    uint8_t buf[4] = {0x80, 0xFF};
    uint8_t i      = 0;
    buf[2]         = 0;
    buf[3]         = mode;
    for (i = 0; i < sizeof(Config_table); i++)
        buf[2] += Config_table[i];
    buf[2] = (~buf[2]) + 1;
    soft_iic_transmit(&scr_touch_iic_handle, Config_table, sizeof(Config_table));
    soft_iic_transmit(&scr_touch_iic_handle, buf, 4);
    return 0;
}

static touch_ic_type read_and_match_screen() {
    uint8_t send_data[]  = {0x81, 0x40};
    uint8_t rece_data[5] = {};
    soft_iic_receive(&scr_touch_iic_handle, send_data, 2, rece_data, 4);

    if (strcmp((char *)rece_data, "9147") == 0) {
        return GT9174;
    }
    else if (strcmp((char *)rece_data, "1158") == 0) {
        return GT1158;
    }
    else
        return UNKNOWN;
}

int lcd_touch_initialize() {
    GPIO_InitTypeDef gp;
    static auto      delay_handle = [](uint32_t delay)
    {
        volatile uint32_t i;
        while (delay--) {
            i = 50;
            while (i)
                i = i - 1;
        }
    };
    scr_touch_iic_port.SCL_pin          = T_SCK_Pin;
    scr_touch_iic_port.SCL_port         = T_SCK_GPIO_Port;
    scr_touch_iic_port.SDA_pin          = T_SDA_Pin;
    scr_touch_iic_port.SDA_port         = T_SDA_GPIO_Port;

    scr_touch_iic_param.delay_handle    = delay_handle;
    scr_touch_iic_param.frequency       = 300000;

    scr_touch_iic_handle.port_setting   = scr_touch_iic_port;
    scr_touch_iic_handle.parameters     = scr_touch_iic_param;
    scr_touch_iic_handle.address_withwr = WRITE_ADDRESS;
    scr_touch_iic_handle.mutex          = xSemaphoreCreateMutex();

    vQueueAddToRegistry((QueueHandle_t)scr_touch_iic_handle.mutex, "iic_handle_mutex");


    gp.Mode  = GPIO_MODE_INPUT;
    gp.Pin   = T_PEN_Pin;
    gp.Pull  = GPIO_PULLUP;
    gp.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_RESET);
    // pull up T_PEN
    HAL_GPIO_Init(T_PEN_GPIO_Port, &gp);

    timer_delay_us(500);

    HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET);

    timer_delay_ms(200);

    touch_ic = read_and_match_screen();
    if (touch_ic == GT9174) {
        gp.Mode = GPIO_MODE_INPUT;
        gp.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(T_PEN_GPIO_Port, &gp);
        // Wait...
        timer_delay_ms(100);

        uint8_t temp[5];
        temp[0] = ((uint32_t)TOUCH_CTRL_REG & 0xff00u) >> 8u;
        temp[1] = ((uint32_t)TOUCH_CTRL_REG & 0x00ffu);
        temp[2] = 0x02;
        soft_iic_transmit(&scr_touch_iic_handle, temp, 3);
        timer_delay_ms(10);
        temp[2] = 0x00;
        soft_iic_transmit(&scr_touch_iic_handle, temp, 3);

        Send_Cfg(0);

        touch_ic_isr_callback = GT9174_isr_callback;
        gp.Mode               = GPIO_MODE_IT_RISING;
        HAL_GPIO_Init(T_PEN_GPIO_Port, &gp);
        HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
        return 0;
    }
    else if (touch_ic == GT1158 || touch_ic == GT1151Q) {
        touch_ic_isr_callback = GT1151Q_1158_isr_callback;
        gp.Mode               = GPIO_MODE_IT_FALLING;
        HAL_GPIO_Init(T_PEN_GPIO_Port, &gp);
        HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
        return 0;
    }
    return -1;
}

void EXTI9_5_IRQHandler() {
    HAL_GPIO_EXTI_IRQHandler(T_PEN_Pin);
}

void gt1158_reset() {
    if (touch_ic == GT1158 || touch_ic == GT1151Q) {
        HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_RESET);
        vTaskDelay(1);
        HAL_GPIO_WritePin(T_CS_GPIO_Port, T_CS_Pin, GPIO_PIN_SET);
    }
}

#include "soft_sccb.hpp"

#define SCCB_SDA(X)  (HAL_GPIO_WritePin(handle->port_setting.SDA_port, handle->port_setting.SDA_pin, (X) ? (GPIO_PIN_SET) : (GPIO_PIN_RESET)))
#define SCCB_SCL(X)  (HAL_GPIO_WritePin(handle->port_setting.SCL_port, handle->port_setting.SCL_pin, (X) ? (GPIO_PIN_SET) : (GPIO_PIN_RESET)))

#define SCCB_SDA_r() (HAL_GPIO_ReadPin(handle->port_setting.SDA_port, handle->port_setting.SDA_pin))
#define SCCB_SCL_r() (HAL_GPIO_ReadPin(handle->port_setting.SCL_port, handle->port_setting.SCL_pin))

static void SCCB_SDA_IN(soft_sccb_handle *handle) {
    GPIO_InitTypeDef init;
    init.Pin = handle->port_setting.SDA_pin;
    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(handle->port_setting.SDA_port, &init);
}
static void SCCB_SDA_OUT(soft_sccb_handle *handle) {
    GPIO_InitTypeDef init;
    init.Pin = handle->port_setting.SDA_pin;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Pull = GPIO_PULLUP;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(handle->port_setting.SDA_port, &init);
}

void SCCB_Start(soft_sccb_handle *handle) {
    SCCB_SDA(1);
    SCCB_SCL(1);
    handle->parameters.delay_handle(5);
    SCCB_SDA(0);
    handle->parameters.delay_handle(5);
    SCCB_SCL(0);
}

void SCCB_Stop(soft_sccb_handle *handle) {
    SCCB_SDA(0);
    handle->parameters.delay_handle(5);
    SCCB_SCL(1);
    handle->parameters.delay_handle(5);
    SCCB_SDA(1);
    handle->parameters.delay_handle(5);
}

void SCCB_No_Ack(soft_sccb_handle *handle) {
    handle->parameters.delay_handle(5);
    SCCB_SDA(1);
    SCCB_SCL(1);
    handle->parameters.delay_handle(5);
    SCCB_SCL(0);
    handle->parameters.delay_handle(5);
    SCCB_SDA(0);
    handle->parameters.delay_handle(5);
}

uint8_t SCCB_WR_Byte(soft_sccb_handle *handle, uint8_t dat) {
    uint8_t j, res;
    for (j = 0; j < 8; j++) {
        if (dat & 0x80u)
            SCCB_SDA(1);
        else
            SCCB_SDA(0);
        dat <<= 1u;
        handle->parameters.delay_handle(5);
        SCCB_SCL(1);
        handle->parameters.delay_handle(5);
        SCCB_SCL(0);
    }
    SCCB_SDA_IN(handle);
    handle->parameters.delay_handle(5);
    SCCB_SCL(1);
    handle->parameters.delay_handle(5);
    if (SCCB_SDA_r())
        res = 1;
    else
        res = 0;
    SCCB_SCL(0);
    SCCB_SDA_OUT(handle);
    return res;
}

uint8_t SCCB_RD_Byte(soft_sccb_handle *handle) {
    uint8_t temp = 0, j;
    SCCB_SDA_IN(handle);    // 设置SDA为输入
    for (j = 8; j > 0; j--) // 循环8次接收数据
    {
        handle->parameters.delay_handle(5);
        SCCB_SCL(1);
        temp = temp << 1u;
        if (SCCB_SDA_r())
            temp++;
        handle->parameters.delay_handle(5);
        SCCB_SCL(0);
    }
    SCCB_SDA_OUT(handle); // 设置SDA为输出
    return temp;
}


uint8_t OV5640_WR_Reg(soft_sccb_handle *handle, uint16_t reg, uint8_t data) {
    uint8_t res = 0;
    SCCB_Start(handle);
    if (SCCB_WR_Byte(handle, handle->address_withwr & 0xFEu))
        res = 1;
    if (SCCB_WR_Byte(handle, reg >> 8u))
        res = 1;
    if (SCCB_WR_Byte(handle, reg))
        res = 1;
    if (SCCB_WR_Byte(handle, data))
        res = 1;
    SCCB_Stop(handle);
    return res;
}

uint8_t OV5640_RD_Reg(soft_sccb_handle *handle, uint16_t reg, uint8_t *rd_data) {
    SCCB_Start(handle);
    if (SCCB_WR_Byte(handle, handle->address_withwr & 0xFEu))
        return 1;
    if (SCCB_WR_Byte(handle, reg >> 8u))
        return 1;
    if (SCCB_WR_Byte(handle, reg))
        return 1;
    SCCB_Stop(handle);

    SCCB_Start(handle);
    if (SCCB_WR_Byte(handle, handle->address_withwr | 0X01u))
        return 1;
    *rd_data = SCCB_RD_Byte(handle);
    SCCB_No_Ack(handle);
    SCCB_Stop(handle);
    return 0;
}

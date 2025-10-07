
#include "pcf8574.hpp"

static soft_iic_handle pcf8574_handle;

#define PCF8574_ADDR      0X40
#define IIC_SCL_Pin       GPIO_PIN_4
#define IIC_SCL_GPIO_Port GPIOH
#define IIC_SDA_Pin       GPIO_PIN_5
#define IIC_SDA_GPIO_Port GPIOH

uint8_t PCF8574_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin              = IIC_SCL_Pin | IIC_SDA_Pin;
    GPIO_InitStruct.Mode             = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull             = GPIO_PULLUP;
    GPIO_InitStruct.Speed            = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    static auto delay_handle = [](uint32_t delay)
    {
        uint32_t i;
        while (delay--) {
            i = 50;
            while (i--)
                ;
        }
    };

    pcf8574_handle.address_withwr          = PCF8574_ADDR;
    pcf8574_handle.port_setting.SCL_port   = IIC_SCL_GPIO_Port;
    pcf8574_handle.port_setting.SCL_pin    = IIC_SCL_Pin;
    pcf8574_handle.port_setting.SDA_port   = IIC_SDA_GPIO_Port;
    pcf8574_handle.port_setting.SDA_pin    = IIC_SDA_Pin;

    pcf8574_handle.parameters.frequency    = 200000;
    pcf8574_handle.parameters.delay_handle = delay_handle;

    int temp                               = soft_iic_transmit(&pcf8574_handle, nullptr, 0);
    PCF8574_WriteOneByte(0XFF);
    return temp;
}

uint8_t PCF8574_ReadOneByte(void) {
    uint8_t temp = 0;
    soft_iic_receive(&pcf8574_handle, nullptr, 0, &temp, 1);
    return temp;
}

void PCF8574_WriteOneByte(uint8_t DataToWrite) {
    uint8_t data = DataToWrite;
    soft_iic_transmit(&pcf8574_handle, &data, 1);
    pcf8574_handle.parameters.delay_handle(20000);
}

void PCF8574_WriteBit(uint8_t bit, uint8_t sta) {
    uint8_t data;
    data = PCF8574_ReadOneByte();
    if (sta == 0)
        data &= ~(1u << bit);
    else
        data |= 1u << bit;
    PCF8574_WriteOneByte(data);
}

uint8_t PCF8574_ReadBit(uint8_t bit) {
    uint8_t data;
    data = PCF8574_ReadOneByte();
    if (data & (1u << bit))
        return 1;
    else
        return 0;
}

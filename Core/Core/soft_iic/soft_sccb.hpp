/*
 * soft_iic.hpp
 *
 *  Created on: Nov 20, 2024
 *      Author: DrL潇湘
 */

#ifndef SOFT_SCCB_SOFT_SCCB_HPP_
#define SOFT_SCCB_SOFT_SCCB_HPP_

#include "main.h"
#include "gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void (*soft_sccb_delay_us_handle)(uint32_t);

    typedef struct soft_sccb_port_origin {
        GPIO_TypeDef *SDA_port;
        GPIO_TypeDef *SCL_port;
        uint16_t SDA_pin;
        uint16_t SCL_pin;
    } soft_sccb_port;

    typedef struct soft_sccb_param_origin {
        soft_sccb_delay_us_handle delay_handle;
        uint32_t frequency;
    } soft_sccb_param;

    typedef struct soft_sccb_handle_origin {
        soft_sccb_port_origin port_setting;
        soft_sccb_param_origin parameters;
        uint8_t address_withwr;
    } soft_sccb_handle;

    void SCCB_Start(soft_sccb_handle *handle);
    void SCCB_Stop(soft_sccb_handle *handle);
    void SCCB_No_Ack(soft_sccb_handle *handle);
    uint8_t SCCB_WR_Byte(soft_sccb_handle *handle, uint8_t dat);
    uint8_t SCCB_RD_Byte(soft_sccb_handle *handle);
    uint8_t OV5640_WR_Reg(soft_sccb_handle *handle, uint16_t reg, uint8_t data);
    uint8_t OV5640_RD_Reg(soft_sccb_handle *handle, uint16_t reg, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* SOFT_IIC_SOFT_IIC_HPP_ */
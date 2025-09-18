/*
 * soft_iic.hpp
 *
 *  Created on: Nov 20, 2024
 *      Author: DrL潇湘
 */

#ifndef SOFT_IIC_SOFT_IIC_HPP_
#define SOFT_IIC_SOFT_IIC_HPP_

#include "main.h"
#include "gpio.h"
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*soft_iic_delay_us_handle)(uint32_t);

typedef struct soft_iic_port_origin {
    GPIO_TypeDef *SDA_port;
    GPIO_TypeDef *SCL_port;
    uint16_t SDA_pin;
    uint16_t SCL_pin;
} soft_iic_port;

typedef struct soft_iic_param_origin {
    soft_iic_delay_us_handle delay_handle;
    uint32_t frequency;
} soft_iic_param;

typedef struct soft_iic_handle_origin {
    soft_iic_port_origin port_setting;
    soft_iic_param_origin parameters;
    uint8_t address_withwr;
    SemaphoreHandle_t mutex;
} soft_iic_handle;

int soft_iic_interact(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length, uint8_t *receive_data, uint32_t receive_length);
int soft_iic_transmit(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length);
int soft_iic_receive(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length, uint8_t *receive_data, uint32_t receive_length);

#ifdef __cplusplus
}
#endif

#endif /* SOFT_IIC_SOFT_IIC_HPP_ */

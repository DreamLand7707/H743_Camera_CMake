/*
 * soft_iic.cpp
 *
 *  Created on: Nov 20, 2024
 *      Author: DrL潇湘
 */

#include "soft_iic.hpp"
#include <cmath>

#define SDA(X) (HAL_GPIO_WritePin(handle->port_setting.SDA_port, handle->port_setting.SDA_pin, (X) ? (GPIO_PIN_SET) : (GPIO_PIN_RESET)))
#define SCL(X) (HAL_GPIO_WritePin(handle->port_setting.SCL_port, handle->port_setting.SCL_pin, (X) ? (GPIO_PIN_SET) : (GPIO_PIN_RESET)))

#define SDA_r() (HAL_GPIO_ReadPin(handle->port_setting.SDA_port, handle->port_setting.SDA_pin))
#define SCL_r() (HAL_GPIO_ReadPin(handle->port_setting.SCL_port, handle->port_setting.SCL_pin))

#define Ack_Gen()   do { SCL(0); SDA(0); delay(halfl_period); SCL(1); delay(halfm_period); SCL(0); delay(halfm_period); SDA(1); } while(0)
#define Ack_nGen()  do { SCL(0); SDA(1); delay(halfl_period); SCL(1); delay(halfm_period); SCL(0); delay(halfm_period); SDA(1); } while(0)
#define Tran_1()    do { SCL(0); SDA(1); delay(halfl_period); SCL(1); delay(halfm_period); } while(0)
#define Tran_0()    do { SCL(0); SDA(0); delay(halfl_period); SCL(1); delay(halfm_period); } while(0)

#define Begin_Gen() do { SDA(1); SCL(1); delay(halfm_period); SDA(0); delay(halfl_period); } while(0)
#define End_Gen()   do { SDA(0); SCL(1); delay(halfm_period); SDA(1); delay(halfl_period); } while(0)
#define Tran(X)     do { if ((X)) { Tran_1(); } else { Tran_0(); } } while(0)
#define Rece()      do { SCL(0); delay(halfm_period); rec = SDA_r(); SCL(1); delay(halfl_period); } while(0)
#define Ack(X)      do { if ((X)) { Ack_Gen(); } else { Ack_nGen(); } } while(0)
#define Ack_Wait()  do { SCL(0); delay(halfm_period); 					\
                         SDA(1); delay(halfl_period); 					\
                         SCL(1); ack = !SDA_r(); delay(halfl_period); 	\
                         SCL(0); delay(halfm_period); } while(0)

#define Trans_8b()  do { 	for (int _ = 0; _ < 8; _++) {		\
                                Tran(temp & 0x80);				\
                                temp <<= 1;						\
                             }									\
                            Ack_Wait();							\
                            if (!ack) {							\
                                End_Gen();						\
                                xSemaphoreGive(handle->mutex); 	\
                                return -1;						\
                            } 									\
                       } while(0)

#define Reces_8b(X)  do {	for (int _ = 0; _ < 8; _++) {	\
                                temp <<= 1;					\
                                Rece();					\
                                if (rec) temp |= 0x01;		\
                            }								\
                            Ack((X));						\
                       } while(0)

int soft_iic_interact(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length, uint8_t *receive_data, uint32_t receive_length) {
    if (handle->address_withwr & 0x01) return soft_iic_receive(handle, send_data, send_length, receive_data, receive_length);
    else return soft_iic_transmit(handle, send_data, send_length);
}

int soft_iic_transmit(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length) {
    uint32_t period = (uint32_t) ceil(1000000.0 / handle->parameters.frequency);
    uint32_t halfm_period = (period >> 1) + (period & 1);
    uint32_t halfl_period = (period >> 1);
    uint8_t ack = 0;
    auto delay = handle->parameters.delay_handle;
    uint8_t temp = handle->address_withwr & 0xFE;
    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    Begin_Gen();
    Trans_8b();
    for (uint32_t i = 0; i < send_length; i++) {
        temp = send_data[i];
        Trans_8b();
    }
    End_Gen();
    xSemaphoreGive(handle->mutex);
    return 0;
}

int soft_iic_receive(soft_iic_handle *handle, const uint8_t *send_data, uint32_t send_length, uint8_t *receive_data, uint32_t receive_length) {
    uint32_t period = (uint32_t) ceil(1000000.0 / handle->parameters.frequency);
    uint32_t halfm_period = (period >> 1) + (period & 1);
    uint32_t halfl_period = (period >> 1);
    uint8_t ack = 0, rec = 0;
    auto delay = handle->parameters.delay_handle;
    uint8_t temp = handle->address_withwr & 0xFE;
    xSemaphoreTake(handle->mutex, portMAX_DELAY);
    Begin_Gen();
    Trans_8b();
    for (uint32_t i = 0; i < send_length; i++) {
        temp = send_data[i];
        Trans_8b();
    }
    temp = handle->address_withwr | 0x01;
    Begin_Gen();
    Trans_8b();
    for (uint32_t i = 0; i < receive_length; i++) {
        temp = 0;
        Reces_8b(i != (receive_length - 1));
        receive_data[i] = temp;
    }
    End_Gen();
    xSemaphoreGive(handle->mutex);
    return 0;
}


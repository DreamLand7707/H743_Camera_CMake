/*
 * lcd_touch.hpp
 *
 *  Created on: Nov 19, 2024
 *      Author: DrL潇湘
 */

#ifndef USER_CODE_LCD_TOUCH_HPP_
#define USER_CODE_LCD_TOUCH_HPP_


#define WRITE_ADDRESS 		0X28
#define READ_ADDRESS 		0X29

#define TOUCH_CTRL_REG 		0X8040
#define TOUCH_CONFIG_REG 	0X8047
#define TOUCH_CKSUM_REG 	0X80FF
#define TOUCH_PID_REG 		0X8140

#define TOUCH_STATUS_REG 	0X814E
#define TOUCH_POINT1_REG 	0X8150
#define TOUCH_POINT2_REG 	0X8158
#define TOUCH_POINT3_REG 	0X8160
#define TOUCH_POINT4_REG 	0X8168
#define TOUCH_POINT5_REG 	0X8170

#include <timer_delay.hpp>
#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "fmc.h"
#include "dcmi.h"
#include "fatfs.h"
#include "ltdc.h"

#include "lvgl.h"
#include "lvgl/examples/porting/lv_port_disp.h"
#include "soft_iic.hpp"

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdlib>

#ifdef __cplusplus
extern "C"
{
#endif

void EXTI9_5_IRQHandler();

extern uint8_t record_touch_valid_num;

#ifdef __cplusplus
}
#endif

void lcd_touch_initialize();
extern soft_iic_port_origin scr_touch_iic_port;
extern soft_iic_param scr_touch_iic_param;
extern soft_iic_handle scr_touch_iic_handle;

#endif /* USER_CODE_LCD_TOUCH_HPP_ */

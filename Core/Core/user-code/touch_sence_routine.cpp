/*
 * flash_screen_routine.cpp
 *
 *  Created on: Nov 23, 2024
 *      Author: DrL潇湘
 */

#include "prj_header.hpp"
#include "SEGGER_RTT.h"

static uint8_t touch_state[3]  = {0x81, 0x4E, 0x00};
static uint8_t data_pos[2]     = {0x81, 0x50};
static uint8_t data_buffer[6]  = {};

touch_point record_touch[5]    = {};
uint8_t record_touch_valid_num = {};

SemaphoreHandle_t sema_screen_been_touched;
TimerHandle_t timer_20_ms_restrain_touch;

static bool touch_invalidate = true;

void timer_20_ms_callback(TimerHandle_t xTimer) {
    touch_invalidate = true;
}

void touch_sence_routine(void const *argument) {
    uint8_t rece_data;
    xSemaphoreTake(sema_flash_screen_routine_start, portMAX_DELAY);

    int touch_number = 0;
    bool not_normal = true, it_high = false, valid = true;

    while (1) {
        xSemaphoreTake(sema_screen_been_touched, portMAX_DELAY);

        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1));
            it_high = HAL_GPIO_ReadPin(T_PEN_GPIO_Port, T_PEN_Pin);
            if (it_high) {
                soft_iic_receive(&scr_touch_iic_handle, touch_state, 2, &rece_data, 1);
                touch_number = rece_data & 0x0F;
                valid        = rece_data & 0x80;
                not_normal   = valid && (!touch_number);

                if (valid)
                    soft_iic_transmit(&scr_touch_iic_handle, touch_state, 3);
                else
                    continue;
                if (not_normal)
                    continue;

                record_touch_valid_num = touch_number;
                data_pos[1]            = 0x50;
                for (int i = 0; i < touch_number; i++) {
                    soft_iic_receive(&scr_touch_iic_handle, data_pos, 2, data_buffer, 6);
                    record_touch[i].x     = uint32_t(data_buffer[0]) | uint32_t(data_buffer[1] << 8);
                    record_touch[i].y     = uint32_t(data_buffer[2]) | uint32_t(data_buffer[3] << 8);
                    record_touch[i].size  = uint32_t(data_buffer[4]) | uint32_t(data_buffer[5] << 8);
                    record_touch[i].state = 1;
                    data_pos[1] += 8;
                }
                for (int i = touch_number; i < 5; i++)
                    record_touch[i].state = 0;
            }
            else {
                for (int i = 0; i < 5; i++)
                    record_touch[i].state = 0;
                break;
            }
        }
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    BaseType_t woken1 = pdFALSE, woken2 = pdFALSE;
    BaseType_t ret;

    switch (GPIO_Pin) {
    case T_PEN_Pin: {
        if (touch_invalidate) {
            touch_invalidate = false;

            ret              = xTimerStartFromISR(timer_20_ms_restrain_touch, &woken1);
            if (ret != pdPASS) {
                // 处理错误
            }

            ret = xSemaphoreGiveFromISR(sema_screen_been_touched, &woken2);
            if (ret != pdPASS) {
                // 处理错误
            }

            // 合并唤醒标志
            portYIELD_FROM_ISR(woken1 || woken2);
        }
        break; // 添加break
    }
    default:
        break;
    }
}

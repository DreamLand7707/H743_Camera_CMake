/*
 * flash_screen_routine.cpp
 *
 *  Created on: Nov 23, 2024
 *      Author: DrL潇湘
 */

#include "prj_header.hpp"
#include "atomic.h"

static uint8_t           touch_state[3] = {0x81, 0x4E, 0x00};
static uint8_t           data_pos[2]    = {0x81, 0x50};
static uint8_t           data_buffer[6] = {};
static TimerHandle_t     timer_20_ms_restrain_touch;
static SemaphoreHandle_t sema_screen_been_touched;

static uint8_t           request_state[3]        = {0x80, 0x44, 0x00};

touch_point              record_touch[10]        = {};
touch_point              record_touch_buffer[10] = {};
uint8_t                  record_touch_valid_num  = {};
touch_ic_isr             touch_ic_isr_callback   = nullptr;

static bool              touch_invalidate        = true;
static void              timer_20_ms_callback(TimerHandle_t xTimer) {
    taskENTER_CRITICAL();
    {
        touch_invalidate = true;
    }
    taskEXIT_CRITICAL();
}
static void GT9174_logic() {
    uint8_t rece_data;
    int     touch_number = 0;
    bool    not_normal = true, it_high = false, valid = true;

    while (true) {
        xSemaphoreTake(sema_screen_been_touched, portMAX_DELAY);

        while (true) {
            // xTracePrint(trace_analyzer_channel2, "=Touch Screen= Begin Touch");
            vTaskDelay(pdMS_TO_TICKS(20));
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

                data_pos[1] = 0x50;
                for (int i = 0; i < touch_number; i++) {
                    soft_iic_receive(&scr_touch_iic_handle, data_pos, 2, data_buffer, 6);
                    record_touch_buffer[i].x     = uint32_t(data_buffer[0]) | uint32_t(data_buffer[1] << 8);
                    record_touch_buffer[i].y     = uint32_t(data_buffer[2]) | uint32_t(data_buffer[3] << 8);
                    record_touch_buffer[i].size  = uint32_t(data_buffer[4]) | uint32_t(data_buffer[5] << 8);
                    record_touch_buffer[i].state = 1;
                    data_pos[1] += 8;
                }
                for (int i = touch_number; i < 5; i++)
                    record_touch_buffer[i].state = 0;

                ATOMIC_ENTER_CRITICAL();
                {
                    record_touch_valid_num = touch_number;
                    for (int i = 0; i < touch_number; i++) {
                        record_touch[i].x     = record_touch[i].x;
                        record_touch[i].y     = record_touch[i].y;
                        record_touch[i].size  = record_touch[i].size;
                        record_touch[i].state = record_touch[i].state;
                    }
                }
                ATOMIC_EXIT_CRITICAL();
            }
            else {
                ATOMIC_ENTER_CRITICAL();
                {
                    record_touch_valid_num = 0;
                    for (int i = 0; i < 5; i++)
                        record_touch[i].state = 0;
                }
                ATOMIC_EXIT_CRITICAL();
                break;
            }
            // xTracePrint(trace_analyzer_channel2, "=Touch Screen= End Touch");
        }
    }
}

static void GT1151Q_1158_logic() { // always falling -> ISR Will Be called always
    uint8_t    rece_data;
    int        touch_number = 0;
    bool       not_normal = true, valid = true;
    BaseType_t should_release = pdFAIL;
    while (true) {
        if (should_release == pdPASS)
            should_release = xSemaphoreTake(sema_screen_been_touched, pdMS_TO_TICKS(20));
        else
            should_release = xSemaphoreTake(sema_screen_been_touched, portMAX_DELAY);

        if (should_release == pdPASS) {
            soft_iic_receive(&scr_touch_iic_handle, touch_state, 2, &rece_data, 1);
            touch_number = rece_data & 0x0F;
            valid        = rece_data & 0x80;
            not_normal   = valid && (!touch_number);

            if (!rece_data) {
                soft_iic_receive(&scr_touch_iic_handle, request_state, 2, &rece_data, 1);
                if (rece_data == 0x00) {
                    ; // pass
                }
                else if (rece_data == 0x03) {
                    gt1158_reset();
                }
                soft_iic_transmit(&scr_touch_iic_handle, request_state, 3);
                taskENTER_CRITICAL();
                {
                    touch_invalidate = true;
                }
                taskEXIT_CRITICAL();
                continue;
            }

            soft_iic_transmit(&scr_touch_iic_handle, touch_state, 3);
            if (not_normal) {
                taskENTER_CRITICAL();
                {
                    touch_invalidate = true;
                }
                taskEXIT_CRITICAL();
                continue;
            }

            data_pos[1] = 0x50;
            for (int i = 0; i < touch_number; i++) {
                soft_iic_receive(&scr_touch_iic_handle, data_pos, 2, data_buffer, 6);
                record_touch_buffer[i].x     = uint32_t(data_buffer[0]) | uint32_t(data_buffer[1] << 8);
                record_touch_buffer[i].y     = uint32_t(data_buffer[2]) | uint32_t(data_buffer[3] << 8);
                record_touch_buffer[i].size  = uint32_t(data_buffer[4]) | uint32_t(data_buffer[5] << 8);
                record_touch_buffer[i].state = 1;
                data_pos[1] += 8;
            }
            for (int i = touch_number; i < 10; i++)
                record_touch[i].state = 0;

            ATOMIC_ENTER_CRITICAL();
            {
                record_touch_valid_num = touch_number;
                for (int i = 0; i < touch_number; i++) {
                    record_touch[i].x     = record_touch[i].x;
                    record_touch[i].y     = record_touch[i].y;
                    record_touch[i].size  = record_touch[i].size;
                    record_touch[i].state = record_touch[i].state;
                }
                touch_invalidate = true;
            }
            ATOMIC_EXIT_CRITICAL();
        }
        else {
            ATOMIC_ENTER_CRITICAL();
            {
                record_touch_valid_num = 0;
                for (int i = 0; i < 10; i++)
                    record_touch[i].state = 0;
                touch_invalidate = true;
            }
            ATOMIC_EXIT_CRITICAL();
            continue;
        }
    }
}

int touch_sence_init() {
    timer_20_ms_restrain_touch = xTimerCreate("timer_20_ms_restrain_touch", pdMS_TO_TICKS(20), pdFALSE, NULL, &timer_20_ms_callback);
    sema_screen_been_touched   = xSemaphoreCreateBinary();

    vQueueAddToRegistry((QueueHandle_t)sema_screen_been_touched, "sema_screen_been_touched");
    vTimerSetTimerNumber(timer_20_ms_restrain_touch, 0);

    lcd_touch_initialize();
    return 0;
}

void touch_sence_routine(void const *argument) {
    xSemaphoreTake(sema_flash_screen_routine_start, portMAX_DELAY);

    if (touch_ic == GT9174) {
        GT9174_logic();
    }
    else if (touch_ic == GT1158 || touch_ic == GT1151Q) {
        GT1151Q_1158_logic();
    }

    while (true)
        ;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
    case T_PEN_Pin: {
        touch_ic_isr_callback();
        break;
    }
    default:
        break;
    }
}

void GT9174_isr_callback() {
    BaseType_t woken1 = pdFALSE, woken2 = pdFALSE;
    if (touch_invalidate) {
        if (xTimerStartFromISR(timer_20_ms_restrain_touch, &woken1) == pdPASS) {
            touch_invalidate = false;
        }
        xSemaphoreGiveFromISR(sema_screen_been_touched, &woken2);
        portYIELD_FROM_ISR(woken1 || woken2);
    }
}

void GT1151Q_1158_isr_callback() {
    BaseType_t woken1 = pdFALSE;
    if (touch_invalidate) {
        touch_invalidate = false;
        xSemaphoreGiveFromISR(sema_screen_been_touched, &woken1);
        portYIELD_FROM_ISR(woken1);
    }
}

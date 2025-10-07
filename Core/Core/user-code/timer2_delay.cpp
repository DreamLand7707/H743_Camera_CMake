/*
 * hard-delay.cpp
 *
 *  Created on: Nov 19, 2024
 *      Author: DrL潇湘
 */

#include "timer2_delay.hpp"

void timer2_delay_us(uint32_t delay_time) {
    if (!delay_time) return;

    CLOSE_BITS(TIM12->CR1, TIM_CR1_CEN); // Close Timer
    CLOSE_BITS(TIM12->CR1, TIM_CR1_ARPE_Msk);
    OPEN_BITS(TIM12->CR1, TIM_CR1_OPM); // Set Pulse Mode
    OPEN_BITS(TIM12->CR1, TIM_CR1_UIFREMAP);

    TIM12->PSC = 239u;
    TIM12->ARR = delay_time - 1;
    SET_BITS(TIM12->EGR, TIM_EGR_UG); // set UG
    TIM12->SR = 0;

    CLOSE_BITS(TIM12->CR1, TIM_CR1_UDIS); // enable UEV
    OPEN_BITS(TIM12->DIER, TIM_DIER_UIE); // enable IT
    OPEN_BITS(TIM12->CR1, TIM_CR1_CEN); // Open Timer

    while (!TIM12->SR) {
    }

    TIM12->SR = 0;

}
// max: 13107ms
void timer2_delay_ms(uint32_t delay_time) {
    if (!delay_time) return;

    CLOSE_BITS(TIM12->CR1, TIM_CR1_CEN); // Close Timer
    CLOSE_BITS(TIM12->CR1, TIM_CR1_ARPE_Msk);
    OPEN_BITS(TIM12->CR1, TIM_CR1_OPM); // Set Pulse Mode
    OPEN_BITS(TIM12->CR1, TIM_CR1_UIFREMAP);

    TIM12->PSC = 47999u;
    TIM12->ARR = delay_time * 5 - 1;
    SET_BITS(TIM12->EGR, TIM_EGR_UG); // set UG
    TIM12->SR = 0;

    CLOSE_BITS(TIM12->CR1, TIM_CR1_UDIS); // enable UEV
    OPEN_BITS(TIM12->DIER, TIM_DIER_UIE); // enable IT
    OPEN_BITS(TIM12->CR1, TIM_CR1_CEN); // Open Timer

    while (!TIM12->SR) {
    }

    TIM12->SR = 0;
}


/*
 * hard-delay.cpp
 *
 *  Created on: Nov 19, 2024
 *      Author: DrL潇湘
 */

#include "timer_delay.hpp"

void timer_delay_us(uint16_t delay_time) {
	if (!delay_time) return;

	CLOSE_BITS(TIM6->CR1, TIM_CR1_CEN); // Close Timer
	CLOSE_BITS(TIM6->CR1, TIM_CR1_ARPE_Msk);
	OPEN_BITS(TIM6->CR1, TIM_CR1_OPM); // Set Pulse Mode
	OPEN_BITS(TIM6->CR1, TIM_CR1_UIFREMAP);

	TIM6->PSC = 239u;
	TIM6->ARR = delay_time - 1;
	SET_BITS(TIM6->EGR, TIM_EGR_UG); // set UG
	TIM6->SR = 0;

	CLOSE_BITS(TIM6->CR1, TIM_CR1_UDIS); // enable UEV
	OPEN_BITS(TIM6->DIER, TIM_DIER_UIE); // enable IT
	OPEN_BITS(TIM6->CR1, TIM_CR1_CEN); // Open Timer

	while (!TIM6->SR) {
	}

	TIM6->SR = 0;

}
// max: 13107ms
void timer_delay_ms(uint16_t delay_time) {
	if (!delay_time) return;

	CLOSE_BITS(TIM6->CR1, TIM_CR1_CEN); // Close Timer
	CLOSE_BITS(TIM6->CR1, TIM_CR1_ARPE_Msk);
	OPEN_BITS(TIM6->CR1, TIM_CR1_OPM); // Set Pulse Mode
	OPEN_BITS(TIM6->CR1, TIM_CR1_UIFREMAP);

	TIM6->PSC = 47999u;
	TIM6->ARR = delay_time * 5 - 1;
	SET_BITS(TIM6->EGR, TIM_EGR_UG); // set UG
	TIM6->SR = 0;

	CLOSE_BITS(TIM6->CR1, TIM_CR1_UDIS); // enable UEV
	OPEN_BITS(TIM6->DIER, TIM_DIER_UIE); // enable IT
	OPEN_BITS(TIM6->CR1, TIM_CR1_CEN); // Open Timer

	while (!TIM6->SR) {
	}

	TIM6->SR = 0;
}


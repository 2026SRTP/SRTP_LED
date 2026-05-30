/*
 * mosfet.c
 *
 *  Created on: Mar 14, 2026
 *      Author: AloysYan
 */

#include "tim.h"
#include "mosfet.h"

// 初始化MOSFET
void MOSFET_Init(void) {
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

// 设置MOSFET占空比
void MOSFET_Update(int count) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, count);
}


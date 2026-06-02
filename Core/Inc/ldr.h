/*
 * ldr.h
 *
 *  Created on: May 21, 2026
 *      Author: AloysYan
 */

#ifndef INC_LDR_H_
#define INC_LDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

#define LDR_ADC_GPIO_PORT        GPIOA
#define LDR_ADC_GPIO_PIN         GPIO_PIN_1
#define LDR_ADC_CHANNEL_NUMBER   1U

#define LDR_ADC_MAX_VALUE        4095U
#define LDR_PWM_MAX_VALUE        999U
#define LDR_SAMPLE_COUNT         10U
#define LDR_HYSTERESIS_THRESHOLD 50U

/* 读取一次 LDR 原始 ADC 值，范围 0~4095。 */
uint16_t LDR_Read(void);

/* 连续采样 10 次并求平均值。 */
uint16_t LDR_ReadAverage(void);

/* 将 ADC 值映射为 PWM 占空比，范围 0~999。 */
uint16_t LDR_ToPWM(uint16_t adc);

#ifdef __cplusplus
}
#endif

#endif /* INC_LDR_H_ */
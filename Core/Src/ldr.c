/*
 *  ldr.c 光敏电阻
 *
 *  Created on: May 21, 2026
 *      Author: AloysYan
 *  
 *  接线说明：
 *    - VCC：3.3V
 *    - ADC：PA1
 */

#include "ldr.h"
#include "adc.h"
#include "tim.h"
#include "ky040.h"

/* 读取一次 ADC 转换结果（使用 HAL 接口）。
   ADC1 已由 CubeMX 在 MX_ADC1_Init() 中初始化（main 启动时调用），
   此处无需重复初始化。 */
uint16_t LDR_Read(void)
{
  uint32_t val = 0U;

  if (HAL_ADC_Start(&hadc1) != HAL_OK)
  {
    return 0U;
  }

  if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
  {
    HAL_ADC_Stop(&hadc1);
    return 0U;
  }

  val = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  return (uint16_t)(val & 0x0FFFU);
}

/* 连续采样 N 次并输出平均值（调用 HAL 版本的 LDR_Read）。 */
uint16_t LDR_ReadAverage(void)
{
  uint32_t sum = 0U;
  uint16_t sample_index = 0U;

  for (sample_index = 0U; sample_index < LDR_SAMPLE_COUNT; sample_index++)
  {
    sum += (uint32_t)LDR_Read();
    /* 小延时让 ADC 有稳定采样时间（可微调） */
    HAL_Delay(2);
  }

  return (uint16_t)(sum / LDR_SAMPLE_COUNT);
}

/* 将环境光 ADC 值反向映射为 PWM，占空比越大表示灯越亮。 */
uint16_t LDR_ToPWM(uint16_t adc)
{
  uint32_t limited_adc = adc;
  uint32_t pwm_value = 0U;
  uint32_t arr = (uint32_t)__HAL_TIM_GET_AUTORELOAD(&htim2);
  uint32_t pwm_max = (arr * KY040_BRIGHTNESS_LIMIT_PCT) / 100U;

  if (limited_adc > LDR_ADC_MAX_VALUE)
  {
    limited_adc = LDR_ADC_MAX_VALUE;
  }

  if (pwm_max > arr)
  {
    pwm_max = arr;
  }

  /* Map ADC (0..4095) directly to PWM (0..pwm_max). On this hardware,
     higher ADC means darker ambient light, so the PWM needs to increase
     with the ADC value. */
  pwm_value = (limited_adc * pwm_max) / LDR_ADC_MAX_VALUE;

  return (uint16_t)pwm_value;
}

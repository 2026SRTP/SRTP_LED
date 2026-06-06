/*
 *  pir.c
 *
 *  Created on: May 25, 2026
 *      Author: AloysYan
 *
 *  接线说明：
 *      - VCC：5V
 *      - OUT：PA2（已由 CubeMX 在 MX_GPIO_Init 中配置为输入）
 */

#include "pir.h"

/**
 * @brief  读取 HC-SR501 原始输出电平
 * @note   PA2 高电平 → 有人，低电平 → 无人
 * @retval PIR_DETECTED     有人
 * @retval PIR_NOT_DETECTED 无人
 */
uint8_t PIR_Read(void)
{
    if (HAL_GPIO_ReadPin(PIR_GPIO_PORT, PIR_GPIO_PIN) == GPIO_PIN_SET)
    {
        return PIR_DETECTED;
    }
    return PIR_NOT_DETECTED;
}

/**
 * @brief  统一人体检测接口
 * @note   当前版本直接透传硬件读数。后续可在此函数内扩展防抖/状态机。
 * @retval PIR_DETECTED     有人
 * @retval PIR_NOT_DETECTED 无人
 */
uint8_t PIR_IsDetected(void)
{
    return PIR_Read();
}

/*
 *  pir.c
 *
 *  Created on: May 25, 2026
 *      Author: AloysYan
 *
 *  接线说明：
 *      - VCC：5V
 *      - OUT：PA2
 */

#include "pir.h"

/* 上电预热起始 tick，0 表示尚未初始化 */
static uint32_t pir_warmup_start = 0U;

/**
 * 初始化 PIR 模块
 * - PA2 已由 CubeMX 在 MX_GPIO_Init() 中配置为输入、无上下拉
 * - 记录预热起始时间戳（非阻塞）
 */
void PIR_Init(void)
{
    /* PA2 已由 CubeMX 在 MX_GPIO_Init() 中配置为输入、无上下拉，
       此处仅记录预热起始时间，无需重复初始化 GPIO */
    pir_warmup_start = HAL_GetTick();
}

/**
 * 读取 HC-SR501 原始输出电平
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
 * 带预热保护的人体检测
 * - 上电 PIR_WARMUP_MS 内 → 强制返回 PIR_NOT_DETECTED（避免误触发）
 * - 预热完成后 → 返回 PIR_Read() 原始结果
 *
 * 注：HC-SR501 使用 BISS0001 芯片做信号调理，输出为干净的数字电平，
 *     当前版本不做额外软件防抖。如需扩展，可在此函数内添加。
 */
uint8_t PIR_IsDetected(void)
{
    /* 预热保护：未初始化或尚在预热期内 */
    
    if (pir_warmup_start == 0U)
    {
        return PIR_NOT_DETECTED;
    }
    if ((HAL_GetTick() - pir_warmup_start) < PIR_WARMUP_MS)
    {
        return PIR_NOT_DETECTED;
    }

    return PIR_Read();
    
    // return PIR_DETECTED;  /* 临时强制返回"有人"，测试逻辑链 */
}

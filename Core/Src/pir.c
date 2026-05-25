/*
 * pir.c
 *
 *  Created on: May 25, 2026
 *      Author: Copilot
 *
 *  HC-SR501 PIR 人体红外感应模块实现
 *  硬件：PA2 ← HC-SR501 OUT，可重复触发模式 (H)
 */

#include "pir.h"

/* 上电预热起始 tick，0 表示尚未初始化 */
static uint32_t pir_warmup_start = 0U;

/**
 * 初始化 PIR 模块
 * - 配置 PA2 为浮空输入（HC-SR501 自带推挽输出驱动，无需上下拉）
 * - 记录预热起始时间戳（非阻塞）
 */
void PIR_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* 配置 PA2 为输入 */
    gpio_init.Pin  = PIR_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(PIR_GPIO_PORT, &gpio_init);

    /* 记录预热起始时间 */
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

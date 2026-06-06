/*
 *  sound.c
 *
 *  Created on: May 26, 2026
 *      Author: AloysYan
 *
 *  硬件说明：板上蓝色电位器用于调节灵敏度阈值
 *
 *  接线说明：
 *    KY-037 VCC → STM32 5V
 *    KY-037 GND → STM32 GND
 *    KY-037 DO  → STM32 PA3
 *    KY-037 AO  → 悬空
 */

#include "sound.h"

/* ---- 软件去抖状态 ---- */
/*
 * 连续高电平计数：每轮主循环（50ms）若读到 HIGH 则 +1，读到 LOW 则清零。
 * 当计数 >= SOUND_DEBOUNCE_CNT 时 Sound_IsDetected() 才返回 SOUND_DETECTED。
 * 这过滤了 <100ms 的尖峰毛刺，同时不损失对真实声音（100ms+）的响应。
 */
static uint8_t sound_high_cnt = 0U;

/**
 * @brief  读取 KY-037 数字输出的原始电平状态（无滤波）
 * @note   直接读取 PA3 引脚电平：
 *         - 高电平（GPIO_PIN_SET） → 环境声音超过阈值，认为"有声"
 *         - 低电平（GPIO_PIN_RESET）→ 环境声音低于阈值，认为"安静"
 * @retval SOUND_DETECTED     有声
 * @retval SOUND_NOT_DETECTED 安静
 */
uint8_t Sound_Read(void)
{
    /* 读取 PA3 引脚电平：高电平表示声音超过 LM393 比较器阈值 */
    if (HAL_GPIO_ReadPin(SOUND_GPIO_PORT, SOUND_GPIO_PIN) == GPIO_PIN_SET)
    {
        return SOUND_DETECTED;
    }
    return SOUND_NOT_DETECTED;
}

/**
 * @brief  带软件去抖的声音检测
 * @note   去抖机制（连续采样确认）：
 *         - 每轮读到 HIGH：计数器 +1，达到 SOUND_DEBOUNCE_CNT(2) 才返回"有声"
 *         - 读到 LOW：计数器立即清零，返回"安静"
 *         效果：滤除 <100ms 的尖峰干扰，但 ≥100ms 的真实声音信号不受影响。
 *         这防止了瞬时噪声反复重置无人计时器导致灯无法关闭的问题。
 * @retval SOUND_DETECTED     确认有声
 * @retval SOUND_NOT_DETECTED 安静
 */
uint8_t Sound_IsDetected(void)
{
    if (Sound_Read() == SOUND_DETECTED)
    {
        /* 读到高电平：连续计数 +1（防溢出保护） */
        if (sound_high_cnt < 255U)
        {
            sound_high_cnt++;
        }

        /* 达到确认阈值 → 确认真实声音 */
        if (sound_high_cnt >= SOUND_DEBOUNCE_CNT)
        {
            return SOUND_DETECTED;
        }
    }
    else
    {
        /* 读到低电平：立刻清零计数器，回到安静状态 */
        sound_high_cnt = 0U;
    }

    return SOUND_NOT_DETECTED;
}

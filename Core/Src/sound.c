/*
 *  sound.c
 *
 *  Created on: May 26, 2026
 *      Author: AloysYan
 *
 *  硬件说明：
 *    板上蓝色电位器用于调节灵敏度阈值
 *
 *  接线说明：
 *    KY-037 VCC → STM32 5V
 *    KY-037 GND → STM32 GND
 *    KY-037 DO  → STM32 PA3
 *    KY-037 AO  → 悬空
 */

#include "sound.h"

/**
 * @brief  初始化声音传感器模块（占位函数）
 * @note   PA3 已由 CubeMX 在 MX_GPIO_Init() 中配置为输入、无上下拉。
 *         此函数保留作为未来扩展（如软件滤波状态初始化）的占位。
 */
void Sound_Init(void)
{
    /* PA3 已由 CubeMX 在 MX_GPIO_Init() 中配置为输入、无上下拉，
       此处无需重复初始化 GPIO。保留此函数作为未来扩展的占位。 */
}

/**
 * @brief  读取 KY-037 数字输出的原始电平状态
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
 * @brief  带扩展能力的统一声音检测接口
 * @note   当前版本直接返回 Sound_Read() 的原始结果（无额外处理）。
 *         KY-037 的 LM393 比较器已提供硬件滞回，输出信号相对干净；
 *         且主循环以 50ms 间隔轮询，本身就提供了一定程度的自然防抖。
 *
 *         如需添加软件层面的抗干扰处理，可在此函数内扩展，例如：
 *         - 要求连续 2~3 次读到高电平才算有效（滤除尖峰噪声）
 *         - 要求在最近 N 毫秒内有至少 K 次高电平才触发（占空比判定）
 *         - 添加最小静音间隔，防止一次长音被重复计数
 *
 * @retval SOUND_DETECTED     有声
 * @retval SOUND_NOT_DETECTED 安静
 */
uint8_t Sound_IsDetected(void)
{
    /* 当前版本：直接透传硬件读数，不做软件滤波 */
    return Sound_Read();
}

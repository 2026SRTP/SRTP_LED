/*
 * sound.c
 *
 *  Created on: May 26, 2026
 *      Author: Copilot
 *
 *  KY-037 声音传感器模块实现
 *  硬件：PA3 ← KY-037 DO（数字输出，LM393 比较器整形）
 *
 *  工作原理：
 *    KY-037 模块内置驻极体麦克风和 LM393 电压比较器。
 *    麦克风拾取环境声音 → 放大 → 与电位器设定的参考电压比较 →
 *    当声音强度超过阈值时 DO 输出高电平，否则输出低电平。
 *    板上蓝色电位器用于调节灵敏度阈值。
 *
 *  接线说明：
 *    KY-037 VCC → STM32 3.3V（模块支持 3.3V~5V 供电）
 *    KY-037 GND → STM32 GND
 *    KY-037 DO  → STM32 PA3
 *    KY-037 AO  → 悬空（本设计不使用模拟输出）
 */

#include "sound.h"

/**
 * @brief  初始化声音传感器 GPIO
 * @note   配置 PA3 为输入、无上下拉。
 *         KY-037 的 DO 引脚由 LM393 比较器推挽驱动，具有确定的电平输出，
 *         因此 MCU 侧不需要内部上下拉电阻，设置为浮空输入即可。
 *         即使 CubeMX 已在 MX_GPIO_Init() 中配置过 PA3，
 *         重复调用 HAL_GPIO_Init 是安全的（HAL 库幂等操作）。
 */
void Sound_Init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* 配置 PA3 为输入模式，无内部上下拉 */
    gpio_init.Pin  = SOUND_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SOUND_GPIO_PORT, &gpio_init);
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

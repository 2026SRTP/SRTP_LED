/*
 * sound.h
 *
 *  Created on: May 26, 2026
 *      Author: Copilot
 *
 *  KY-037 声音传感器模块接口
 *  硬件：PA3 ← KY-037 DO（数字输出），仅用于判定有无声音
 *  板上 LM393 比较器：有声时 DO 输出高电平，安静时输出低电平
 */

#ifndef INC_SOUND_H_
#define INC_SOUND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

/* ---------- 引脚定义 ---------- */
/* KY-037 数字输出引脚，连接到 STM32 的 PA3 */
#define SOUND_GPIO_PORT         GPIOA
#define SOUND_GPIO_PIN          GPIO_PIN_3

/* ---------- 检测状态 ---------- */
/* KY-037 DO 输出高电平时表示检测到声音 */
#define SOUND_DETECTED          1U
/* KY-037 DO 输出低电平时表示环境安静 */
#define SOUND_NOT_DETECTED      0U

/* ---------- 接口函数 ---------- */

/**
 * @brief  初始化声音传感器模块
 * @note   配置 PA3 为浮空输入模式（KY-037 自带推挽输出，无需上下拉）
 *         若 CubeMX 已在 MX_GPIO_Init() 中初始化 PA3，此函数仍可安全调用
 */
void Sound_Init(void);

/**
 * @brief  读取 KY-037 数字输出的原始电平
 * @retval SOUND_DETECTED     检测到声音（PA3 为高电平）
 * @retval SOUND_NOT_DETECTED 未检测到声音（PA3 为低电平）
 */
uint8_t Sound_Read(void);

/**
 * @brief  带扩展接口的声音检测函数
 * @note   当前版本直接返回 Sound_Read() 的原始结果
 *         后续可在此函数内添加软件防抖、最小持续检测时间等逻辑
 *         例如：要求连续 N 次读到高电平才算有效，过滤瞬时噪声
 * @retval SOUND_DETECTED     检测到声音
 * @retval SOUND_NOT_DETECTED 未检测到声音
 */
uint8_t Sound_IsDetected(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_SOUND_H_ */

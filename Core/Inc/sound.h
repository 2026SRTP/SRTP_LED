/*
 * sound.h
 *
 *  Created on: May 26, 2026
 *      Author: AloysYan
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

/* ---------- 软件滤波参数 ---------- */
/*
 * 去抖确认次数：需要连续读到多少次 HIGH 才确认"有声"。
 * 主循环周期为 50ms，因此实际确认时间 = SOUND_DEBOUNCE_CNT × 50ms。
 *
 * 阈值选择依据（平衡关灯防噪与人在防灭）：
 *   - 阈值=1：无滤波，50ms 毛刺即可触发 → 噪声可能导致关灯超时被不断重置
 *   - 阈值=2：100ms 确认 ← 推荐。滤除 50ms 尖峰，但能捕获 100ms+ 的真实声音
 *   - 阈值=3：150ms 确认，更稳但可能漏掉短暂声音（如轻敲键盘）
 *
 * 当前选用 2：既能过滤电气毛刺，又不会因阈值过严导致人静止时
 * 因短暂无声而触发 5 秒超时误关灯。
 */
#define SOUND_DEBOUNCE_CNT      2U

/* ---------- 接口函数 ---------- */

/**
 * @brief  读取 KY-037 数字输出的原始电平（无滤波）
 * @retval SOUND_DETECTED     检测到声音（PA3 为高电平）
 * @retval SOUND_NOT_DETECTED 未检测到声音（PA3 为低电平）
 */
uint8_t Sound_Read(void);

/**
 * @brief  带软件去抖的声音检测
 * @note   需连续 SOUND_DEBOUNCE_CNT 次（当前为 2 次 = 100ms）读到 HIGH 才确认有声；
 *         任何一次读到 LOW 立即清零计数器、返回"安静"。
 *         此设计可有效滤除电磁干扰引起的单次尖峰脉冲。
 * @retval SOUND_DETECTED     确认有声
 * @retval SOUND_NOT_DETECTED 安静（含未达确认阈值的瞬时高电平）
 */
uint8_t Sound_IsDetected(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_SOUND_H_ */

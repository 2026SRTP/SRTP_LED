/*
 * pir.h
 *
 *  Created on: May 25, 2026
 *      Author: AloysYan
 *
 */

#ifndef INC_PIR_H_
#define INC_PIR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

/* ---------- 引脚定义 ---------- */
#define PIR_GPIO_PORT           GPIOA
#define PIR_GPIO_PIN            GPIO_PIN_2

/* ---------- 检测状态 ---------- */
#define PIR_DETECTED            1U   /* 检测到人体 */
#define PIR_NOT_DETECTED        0U   /* 未检测到人体 */

/* ---------- 接口函数 ---------- */

/** 读取 PA2 原始电平，返回 PIR_DETECTED 或 PIR_NOT_DETECTED */
uint8_t PIR_Read(void);

/** 人体检测（当前版本直接透传硬件读数，后续可在此扩展防抖） */
uint8_t PIR_IsDetected(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_PIR_H_ */

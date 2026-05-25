/*
 * pir.h
 *
 *  Created on: May 25, 2026
 *      Author: Copilot
 *
 *  HC-SR501 PIR 人体红外感应模块接口
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
#define PIR_DETECTED            1U
#define PIR_NOT_DETECTED        0U

/* ---------- 时间参数 ---------- */
#define PIR_WARMUP_MS           1000U   /* 上电预热时间（HC-SR501 需约 30~60s） */
#define PIR_OFF_DELAY_MS        5000U    /* 无人后延时关灯时间 */

/* ---------- 接口函数 ---------- */

/** 初始化 PIR 模块（记录预热起始时间，配置 GPIO） */
void PIR_Init(void);

/** 读取 PA2 原始电平，返回 PIR_DETECTED 或 PIR_NOT_DETECTED */
uint8_t PIR_Read(void);

/**
 * 带预热保护的人体检测
 * - 上电后 PIR_WARMUP_MS 毫秒内强制返回 PIR_NOT_DETECTED
 * - 预热完成后返回 PIR_Read() 结果
 * - 后续可在此函数内扩展防抖 / 状态机
 */
uint8_t PIR_IsDetected(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_PIR_H_ */

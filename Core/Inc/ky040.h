/*
 * ky040.h
 *
 *  Created on: Mar 14, 2026
 *      Author: AloysYan
 *
 *  KY-040 旋转编码器接口
 *
 *  三模式按键切换：
 *    模式0 (OFF)    — 强制关灯，传感器和编码器均不响应
 *    模式1 (MANUAL) — 编码器旋钮直接控制亮度，传感器不参与
 *    模式2 (AUTO)   — PIR + 声音 + LDR 全自动控制
 *    每次按键按下，mode = (mode + 1) % 3，循环切换
 */

#ifndef INC_KY040_H_
#define INC_KY040_H_

/* 亮度上限百分比：PWM 最大值 = TIM2 ARR × 此百分比 */
#define KY040_BRIGHTNESS_LIMIT_PCT   20U

/* ---------- 三模式常量 ---------- */
#define MODE_OFF     0   /* 关灯模式：强制灭灯，忽略所有传感器和编码器 */
#define MODE_MANUAL  1   /* 手动模式：编码器直接控制亮度，传感器不参与 */
#define MODE_AUTO    2   /* 自动模式：PIR 开灯 + 声音续命 + LDR 调光 + 超时关灯 */

/* ---------- 全局变量 ---------- */
extern int count;   /* 编码器当前亮度值（0 ~ pwm_max），由 KY040_Update() 实时更新 */
extern int mode;    /* 当前工作模式，取值为 MODE_OFF / MODE_MANUAL / MODE_AUTO */

/* ---------- 接口函数 ---------- */
void KY040_Init(void);
void KY040_Update(void);
void KY040_Key(void);

#endif /* INC_KY040_H_ */

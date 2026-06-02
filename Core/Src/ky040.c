/*
 *  ky040.c 旋转编码器
 *
 *  Created on: Mar 14, 2026
 *      Author: AloysYan
 * 
 * 	接线说明：
 * 		- VCC：3.3V
 *		- CLK：PA9
 *		- DT：PA8
 *		- SW：PB15
 */

#include "tim.h"
#include "main.h"
#include "ky040.h"

int count = 0;
int light_on = 0;
static int last_encoder_raw = 0;
static int brightness_units = 0;

// 可调参数：旋钮转满一圈对应的编码器步数（KY040常见约20步/圈）
#define KY040_STEPS_PER_REV          20

// 初始化编码器
void KY040_Init() {
	// ky040编码器初始化
	HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
	// 初始化计数值和LED状态
	count = 0;
	light_on = 0;
	__HAL_TIM_SET_COUNTER(&htim1, 0);
	last_encoder_raw = 0;
	brightness_units = 0;
}

// 更新编码器状态并调整LED亮度
void KY040_Update() {
	int encoder_raw = (int)__HAL_TIM_GET_COUNTER(&htim1);
	int encoder_range = (int)__HAL_TIM_GET_AUTORELOAD(&htim1) + 1;
	int delta = encoder_raw - last_encoder_raw;
	int pwm_max = ((int)__HAL_TIM_GET_AUTORELOAD(&htim2) * KY040_BRIGHTNESS_LIMIT_PCT) / 100;
	int brightness_units_max = pwm_max * KY040_STEPS_PER_REV;

	// 处理计数器回绕，保证每次只得到小步进增量
	if (delta > (encoder_range / 2)) {
		delta -= encoder_range;
	} else if (delta < -(encoder_range / 2)) {
		delta += encoder_range;
	}

	// 将旋钮位移映射到亮度：转满一圈到达最大亮度
	brightness_units += delta * pwm_max;

	// 饱和限幅，避免亮度在边界发生回绕
	if (brightness_units < 0) {
		brightness_units = 0;
	} else if (brightness_units > brightness_units_max) {
		brightness_units = brightness_units_max;
	}

	// 把中间量还原为PWM比较值（四舍五入）
	count = (brightness_units + (KY040_STEPS_PER_REV / 2)) / KY040_STEPS_PER_REV;
	if (count > pwm_max) {
		count = pwm_max;
	}

	last_encoder_raw = encoder_raw;
}

// 按键处理函数
void KY040_Key() {
	// 按键按下
	if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET) {
		// 防抖处理
	    HAL_Delay(10);
	    if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET) {
	        // 切换LED状态
			light_on = !light_on;
	    }
		// 等待按键释放
	    while (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET);
	}
}

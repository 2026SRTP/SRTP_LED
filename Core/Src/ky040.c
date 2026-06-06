/*
 *  ky040.c 旋转编码器
 *
 *  Created on: Mar 14, 2026
 *      Author: AloysYan
 *
 *  接线说明：
 *      - VCC：3.3V
 *      - CLK：PA9
 *      - DT：PA8
 *      - SW：PB15
 *
 *  三模式按键：
 *      上电默认为 MODE_OFF（关灯）。
 *      每次短按按键，mode 在 OFF → MANUAL → AUTO → OFF 之间循环。
 */

#include "tim.h"
#include "main.h"
#include "ky040.h"

/* ---- 全局变量 ---- */
int count = 0;            /* 编码器当前亮度值，范围 0 ~ pwm_max */
int mode  = MODE_OFF;     /* 当前工作模式，上电默认关灯 */

/* ---- 编码器内部状态 ---- */
static int last_encoder_raw = 0;    /* 上一次 TIM1 编码器原始计数值 */
static int brightness_units = 0;    /* 亮度中间累积量（精细单位） */

/* 旋钮转满一圈对应的编码器步数（KY-040 典型约 20 步/圈） */
#define KY040_STEPS_PER_REV          20

/**
 * @brief  初始化旋转编码器
 * @note   启动 TIM1 编码器接口，将计数值和亮度状态全部复位到零
 */
void KY040_Init(void)
{
    /* 启动 TIM1 编码器模式，双通道（A/B 相） */
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);

    /* 所有状态复位 */
    count = 0;
    mode  = MODE_OFF;                       /* 上电进入关灯模式 */
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    last_encoder_raw = 0;
    brightness_units = 0;
}

/**
 * @brief  更新编码器状态，将旋钮位移映射为亮度值
 * @note   每轮主循环调用一次。
 *         无论当前处于哪个模式，count 始终被实时更新，
 *         这样切换到手动模式时亮度值总是最新的。
 */
void KY040_Update(void)
{
    int encoder_raw   = (int)__HAL_TIM_GET_COUNTER(&htim1);
    int encoder_range = (int)__HAL_TIM_GET_AUTORELOAD(&htim1) + 1;
    int delta         = encoder_raw - last_encoder_raw;

    /* PWM 最大值受亮度上限百分比约束 */
    int pwm_max = ((int)__HAL_TIM_GET_AUTORELOAD(&htim2) * KY040_BRIGHTNESS_LIMIT_PCT) / 100;
    int brightness_units_max = pwm_max * KY040_STEPS_PER_REV;

    /* 处理编码器计数器回绕（TIM1 为循环计数器，需解绕） */
    if (delta > (encoder_range / 2)) {
        delta -= encoder_range;
    } else if (delta < -(encoder_range / 2)) {
        delta += encoder_range;
    }

    /* 将旋钮位移量累积到中间精度变量（转满一圈 = 到达最大亮度） */
    brightness_units += delta * pwm_max;

    /* 饱和限幅，防止在边界处发生回绕 */
    if (brightness_units < 0) {
        brightness_units = 0;
    } else if (brightness_units > brightness_units_max) {
        brightness_units = brightness_units_max;
    }

    /* 将累积量四舍五入还原为 PWM 比较值 */
    count = (brightness_units + (KY040_STEPS_PER_REV / 2)) / KY040_STEPS_PER_REV;
    if (count > pwm_max) {
        count = pwm_max;
    }

    last_encoder_raw = encoder_raw;
}

/**
 * @brief  处理编码器按键，实现三模式循环切换
 * @note   按键逻辑：
 *         - 检测到 PB15 低电平（按键按下）后，延时 10ms 软件防抖
 *         - 防抖确认有效后，mode = (mode + 1) % 3，循环 OFF→MANUAL→AUTO→OFF
 *         - 等待按键释放（带 500ms 超时保护，防止引脚异常导致死锁）
 */
void KY040_Key(void)
{
    /* 按键按下（PB15 配置为内部上拉，按下时为低电平） */
    if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
    {
        /* 软件防抖：延时 10ms 后再次读取确认 */
        HAL_Delay(10);
        if (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
        {
            /*
             * 确认按键有效：三模式循环切换
             * OFF(0) → MANUAL(1) → AUTO(2) → OFF(0) → ...
             */
            mode = (mode + 1) % 3;
        }

        /*
         * 等待按键释放，带超时保护（500ms）。
         * 若 PB15 因硬件异常（引脚悬空、短路、干扰）持续为低电平，
         * 超时后强制退出，防止整个主循环被永久阻塞。
         * 正常情况下按键释放（PB15 回到高电平）后立即退出循环。
         */
        uint32_t key_release_start = HAL_GetTick();
        while (HAL_GPIO_ReadPin(Key_GPIO_Port, Key_Pin) == GPIO_PIN_RESET)
        {
            if ((HAL_GetTick() - key_release_start) > 500U)
            {
                break;  /* 超时保护：500ms 后强制退出，防止死锁 */
            }
        }
    }
}

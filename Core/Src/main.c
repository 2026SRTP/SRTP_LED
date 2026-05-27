/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ldr.h"
#include "ky040.h"
#include "mosfet.h"
#include "pir.h"
#include "sound.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* ---- LDR 自动调光相关变量 ---- */
static uint16_t last_adc = 0U;  /* 上一次 LDR 读取的 ADC 值，用于滞回比较 */
static uint16_t last_pwm = 0U;  /* 上一次计算出的 PWM 占空比输出值 */

/* ---- 手动优先超时参数 ---- */
/* 编码器操作后，手动模式保持的时长（单位：毫秒）。
   超时后自动恢复到 PIR+声音 自动控制模式 */
#define LDR_MANUAL_OVERRIDE_MS 5000U

/* ---- 手动模式状态追踪 ---- */
static int      last_count_snapshot = -1;   /* 上一轮编码器计数值的快照，用于检测旋钮是否被操作 */
static uint32_t last_manual_tick    = 0U;   /* 最后一次手动操作（编码器动作）的时间戳 */

/* ---- PIR + 声音 复合自动控制相关变量 ---- */
/*
 * 自动关灯超时时间（单位：毫秒）。
 * 灯亮状态下，若连续超过此时间既没有检测到人体（PIR）也没有检测到声音，
 * 则判定"人已离开且环境安静"，自动关灯。
 * 当前设定为 5 秒，可根据实际使用场景调整。
 */
#define AUTO_OFF_TIMEOUT_MS  5000U

/*
 * 最近一次活动时间戳（单位：milliseconds，基于 HAL_GetTick()）。
 * "活动"定义为：PIR 检测到人体 或 KY-037 检测到声音。
 * 每次检测到活动时，将此变量更新为当前系统 tick。
 * 灯亮时，若 (当前tick - last_activity_tick) > AUTO_OFF_TIMEOUT_MS，则关灯。
 */
static uint32_t last_activity_tick = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  KY040_Init();       /* 初始化 KY-040 旋转编码器（亮度手动调节 + 开关） */
  MOSFET_Init();       /* 初始化 MOSFET PWM 输出（TIM2 CH1 驱动 LED 灯带） */
  PIR_Init();          /* 初始化 HC-SR501 人体红外传感器（PA2 输入） */
  Sound_Init();        /* 初始化 KY-037 声音传感器（PA3 输入，数字输出） */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* 更新编码器状态并处理按键输入 */
    KY040_Update();
    KY040_Key();

    /* ================================================================
     * 手动控制检测（编码器优先）
     * ================================================================
     * 当用户旋转编码器时，立即进入"手动优先模式"：
     *   - 亮度由编码器位置直接决定，不受 PIR/声音/LDR 影响
     *   - 手动模式持续 LDR_MANUAL_OVERRIDE_MS（5 秒），超时后自动切回自动模式
     *   - 灯开关状态（light_on）仍由 KY040_Key() 按键控制
     * ================================================================ */
    if (last_count_snapshot != count) {
        /* 编码器计数值发生变化 —— 用户正在操作旋钮，进入手动优先 */
        last_count_snapshot = count;
        last_manual_tick = HAL_GetTick();   /* 记录手动操作时刻，启动超时计时 */

        /* 根据当前开关状态输出对应亮度 */
        if (light_on) {
            MOSFET_Update(count);
        } else {
            MOSFET_Update(0);
        }
    } else {
        /*
         * 编码器未被操作：检查是否仍在手动优先超时窗口内。
         * 若手动操作后不到 LDR_MANUAL_OVERRIDE_MS（5 秒），继续维持手动值，
         * 给用户充足的调节时间，避免刚调好就被自动模式覆盖。
         */
        if (last_manual_tick != 0U &&
            ((HAL_GetTick() - last_manual_tick) < LDR_MANUAL_OVERRIDE_MS)) {
            if (light_on) {
                MOSFET_Update(count);
            } else {
                MOSFET_Update(0);
            }
        } else {
            /* ============================================================
             * 自动模式：PIR + 声音 复合人体存在检测 + LDR 自动调光
             * ============================================================
             *
             * 控制策略（三段式）：
             *
             *   [1] 读取传感器状态
             *       读取 PIR（人体红外）和 KY-037 声音传感器的当前状态。
             *       若任一方检测到活动，刷新 last_activity_tick 时间戳。
             *
             *   [2] 灯灭状态 —— 仅允许 PIR 开灯
             *       设计意图：防止门外噪音、隔壁房间声音等误触发开灯。
             *       只有人体红外传感器确认"有人进入"时才允许开灯。
             *       开灯后立即根据 LDR 环境光传感器计算合适的 PWM 亮度。
             *
             *   [3] 灯亮状态 —— PIR 或声音均可"续命"，超时关灯
             *       设计意图：人在房间内可能有静止不动的时候（如坐着看书），
             *       此时 PIR 可能无法检测到。但人会有自然的声音（翻书、咳嗽、
             *       打字等），声音传感器可捕捉这些信号来维持灯光。
             *       只有当连续 AUTO_OFF_TIMEOUT_MS（5 秒）既无 PIR 也无声音
             *       时，才判定"人已离开"并关灯。
             *
             *   LDR 自动调光：灯亮期间持续读取光敏电阻，根据环境亮度自动
             *   调节 PWM 占空比。使用滞回阈值（LDR_HYSTERESIS_THRESHOLD）
             *   避免 ADC 微小波动导致灯光闪烁。
             * ============================================================ */

            /* [1] 读取两个传感器的当前状态 */
            uint8_t pir_detected = PIR_IsDetected();   /* 人体红外：有人=1，无人=0 */
            uint8_t snd_detected = Sound_IsDetected();  /* 声音检测：有声=1，安静=0 */

            /*
             * 活动计时器刷新：
             * PIR 或声音任意一个检测到活动，都将 last_activity_tick 更新为当前时刻。
             * 这相当于"重置无人计时器"——只要还有人活动的迹象，就不开始关灯倒计时。
             */
            if (pir_detected == PIR_DETECTED || snd_detected == SOUND_DETECTED) {
                last_activity_tick = HAL_GetTick();
            }

            if (light_on == 0U) {
                /* ========================================================
                 * [2] 灯灭状态：仅 PIR 可触发开灯，声音不能
                 * ======================================================== */
                if (pir_detected == PIR_DETECTED) {
                    /* PIR 检测到有人进入 → 开灯 */
                    light_on = 1U;
                    /* 首次开灯时记录活动时间戳，启动关灯倒计时 */
                    last_activity_tick = HAL_GetTick();

                    /*
                     * 立即读取一次 LDR 环境光并计算 PWM 输出。
                     * 不等待下一轮循环，确保开灯响应迅速、无延迟。
                     */
                    uint16_t adc_value = LDR_ReadAverage();
                    last_adc = adc_value;
                    last_pwm = LDR_ToPWM(adc_value);
                    MOSFET_Update((int)last_pwm);
                } else {
                    /*
                     * 无人（可能有声也可能无声）→ 保持关灯。
                     * 声音传感器在此状态下被忽略：门外噪音不会误开灯。
                     */
                    MOSFET_Update(0);
                }
            } else {
                /* ========================================================
                 * [3] 灯亮状态：PIR 或声音可续命，超时自动关灯
                 * ======================================================== */

                /*
                 * 超时关灯判定：
                 * 若距离上一次活动（PIR 或声音触发）已经超过 AUTO_OFF_TIMEOUT_MS，
                 * 说明房间内连续指定时间没有任何人体活动迹象，
                 * 判定"人已离开"，执行关灯。
                 */
                if ((last_activity_tick != 0U) &&
                    ((HAL_GetTick() - last_activity_tick) > AUTO_OFF_TIMEOUT_MS)) {
                    /* 超时：关灯，清零相关状态 */
                    MOSFET_Update(0);
                    light_on = 0U;
                    last_activity_tick = 0U;   /* 重置活动计时器，等待下次 PIR 触发 */
                } else {
                    /*
                     * 未超时：人在房间里（或刚离开不到 5 秒）。
                     * 继续执行 LDR 环境光自动调光。
                     */

                    /* 读取当前环境光 ADC 值 */
                    uint16_t adc_value = LDR_ReadAverage();

                    /*
                     * 滞回比较（Hysteresis）：
                     * 仅当 ADC 变化量超过 LDR_HYSTERESIS_THRESHOLD 时才更新 PWM 输出。
                     * 这避免了 ADC 采样噪声导致的灯光微小闪烁，
                     * 使亮度调节更平滑稳定。
                     *
                     * 注：首次进入此分支时 last_pwm 为 0，滞回条件会自动满足，
                     *     因此首次会正常输出亮度。
                     */
                    uint16_t delta = (adc_value > last_adc)
                                     ? (adc_value - last_adc)
                                     : (last_adc - adc_value);

                    if (delta > LDR_HYSTERESIS_THRESHOLD) {
                        /* 环境光变化超过阈值 → 更新 PWM 输出 */
                        last_adc = adc_value;
                        last_pwm = LDR_ToPWM(adc_value);
                        MOSFET_Update((int)last_pwm);
                    } else {
                        /* 环境光变化不大 → 维持上一次的 PWM 输出，避免闪烁 */
                        MOSFET_Update((int)last_pwm);
                    }
                }
            }
        }
    }

    /*
     * 主循环延时 50 毫秒。
     * 说明：
     *   - 50ms 的轮询周期对于人体检测（PIR 响应约 100~300ms）足够快
     *   - 对于声音检测，50ms 能可靠捕获持续时间 ≥100ms 的声音事件（如拍手、说话）
     *   - 此延时同时决定了 PWM 更新频率（20Hz），远超人眼对灯光闪烁的感知阈值
     */
    HAL_Delay(50);
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

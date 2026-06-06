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

/*
 * 编码器计数值快照，用于检测"自动模式下编码器是否被拧动"。
 * 自动模式下若 count 与快照不一致，说明用户拧了旋钮 → 切到手动模式。
 * 关灯模式和手动模式下每轮同步快照，确保切换到自动时不会误触发。
 */
static int last_count_snapshot = 0;

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

/*
 * 自动模式下的灯开关状态（仅 MODE_AUTO 下有效）。
 * 0 = 自动模式灯灭，1 = 自动模式灯亮。
 * 与全局 mode 变量不同：mode 表示用户选择的工作模式（0/1/2），
 * auto_light_on 只追踪自动模式内部灯光是否因传感器触发而亮起。
 */
static uint8_t auto_light_on = 0U;

/*
 * 上一轮的工作模式，用于检测模式切换事件。
 * 当检测到从其他模式进入 MODE_AUTO 时，强制重置自动模式内部状态，
 * 避免因 auto_light_on / last_activity_tick 残留值导致的异常行为。
 */
static int last_mode = MODE_OFF;

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
    /* ================================================================
     * 每轮循环：更新编码器 → 检测按键 → 按模式分发控制
     * ================================================================
     * KY040_Update() 始终运行，确保 count 实时反映编码器位置。
     * KY040_Key() 检测按键并循环切换 mode（OFF→MANUAL→AUTO）。
     * 然后根据当前 mode 执行对应的控制逻辑。
     * ================================================================ */
    KY040_Update();
    KY040_Key();

    /*
     * 模式切换检测：
     * 当用户通过按键从其他模式进入 MODE_AUTO 时，强制重置自动模式内部状态
     * （auto_light_on=0, last_activity_tick=0），确保每次进入自动模式都
     * 从"灯灭子状态"开始，立即根据传感器状态重新判断是否开灯。
     * 这解决了从"模式2→拧编码器→模式1→按键→模式2"路径下，
     * auto_light_on 残留为 1、last_activity_tick 过期导致立即关灯的 bug。
     */
    if (mode != last_mode && mode == MODE_AUTO)
    {
        auto_light_on     = 0U;
        last_activity_tick = 0U;
    }
    last_mode = mode;   /* 记录本轮模式，供下轮检测切换 */

    switch (mode)
    {
    /* =================================================================
     * 模式 0 — OFF（关灯）
     * =================================================================
     * 强制关灯，PWM 输出为 0。
     * 编码器和所有传感器均被忽略。
     * 同步编码器快照，为后续切换到自动模式做准备。
     * ================================================================= */
    case MODE_OFF:
        MOSFET_Update(0);
        last_count_snapshot = count;   /* 同步快照，防止切自动时误触发 */
        break;

    /* =================================================================
     * 模式 1 — MANUAL（手动调光）
     * =================================================================
     * 灯光亮度完全由编码器旋钮位置决定，PWM = count。
     * PIR、声音、LDR 传感器全部不参与控制。
     * 此模式下拧旋钮实时改变亮度，无超时限制。
     * 同步编码器快照，为后续切换到自动模式做准备。
     * ================================================================= */
    case MODE_MANUAL:
        MOSFET_Update(count);
        last_count_snapshot = count;   /* 同步快照，防止切自动时误触发 */
        break;

    /* =================================================================
     * 模式 2 — AUTO（全自动控制）
     * =================================================================
     * 控制策略：
     *   [A] 编码器检测：若旋钮被拧动 → 立即切到手动模式（用户接管）
     *   [B] 传感器读取：PIR（人体红外） + KY-037（声音）
     *   [C] 活动计时：PIR 或声音任一触发 → 刷新 last_activity_tick
     *   [D] 灯灭子状态：仅 PIR 可开灯，声音不能（防门外噪音误触发）
     *       开灯时立即用 LDR 环境光传感器计算合适的 PWM 亮度
     *   [E] 灯亮子状态：PIR 或声音均可续命；连续 5 秒无活动则关灯
     *       期间持续 LDR 自动调光（带滞回，防闪烁）
     * ================================================================= */
    case MODE_AUTO:
    default:
        /*
         * [A] 编码器拧动检测：
         * 比较当前 count 与上一轮快照，若不一致说明用户拧了旋钮。
         * 立即切到模式 1（手动），用当前编码器值直接设置亮度。
         */
        if (last_count_snapshot != count)
        {
            last_count_snapshot = count;
            mode = MODE_MANUAL;         /* 用户接管 → 切手动 */
            MOSFET_Update(count);
            break;                      /* 跳出 switch，本轮不执行自动逻辑 */
        }

        /*
         * [B] 读取两个传感器的当前状态。
         * PIR_IsDetected() 内部包含 1 秒预热保护（测试用，正式需 >30s）。
         */
        uint8_t pir_detected = PIR_IsDetected();   /* 人体红外：有人=1，无人=0 */
        uint8_t snd_detected = Sound_IsDetected();  /* 声音检测：有声=1，安静=0 */

        /*
         * [C] 活动计时器刷新：
         * PIR 或声音任意一个检测到活动，都将 last_activity_tick 更新为当前时刻。
         * 相当于"有人活动就重置无人倒计时"。
         */
        if (pir_detected == PIR_DETECTED || snd_detected == SOUND_DETECTED)
        {
            last_activity_tick = HAL_GetTick();
        }

        /* ---- 自动模式内部状态机 ---- */
        if (auto_light_on == 0U)
        {
            /* ========================================================
             * [D] 灯灭子状态 —— 仅允许 PIR 开灯
             * ========================================================
             * 设计意图：防止门外噪音、隔壁房间声音等误触发开灯。
             * 只有人体红外确认"有人进入"时才允许开灯。
             * 开灯后立即根据 LDR 环境光传感器计算合适的 PWM 亮度。
             * ======================================================== */
            if (pir_detected == PIR_DETECTED)
            {
                /* PIR 检测到有人进入 → 自动开灯 */
                auto_light_on = 1U;
                /* 记录活动时间戳，启动关灯倒计时 */
                last_activity_tick = HAL_GetTick();

                /*
                 * 立即读取 LDR 环境光并计算 PWM 输出。
                 * 不等待下一轮循环，确保开灯响应迅速、无延迟。
                 */
                uint16_t adc_value = LDR_ReadAverage();
                last_adc = adc_value;
                last_pwm = LDR_ToPWM(adc_value);
                MOSFET_Update((int)last_pwm);
            }
            else
            {
                /*
                 * 无人（可能有声也可能无声）→ 保持关灯。
                 * 声音传感器在此状态下被忽略：门外噪音不会误开灯。
                 */
                MOSFET_Update(0);
            }
        }
        else
        {
            /* ========================================================
             * [E] 灯亮子状态 —— PIR 或声音可续命，超时自动关灯
             * ========================================================
             * 设计意图：人在房间内可能静止不动（如坐着看书），
             * 此时 PIR 可能无法检测到。但人会有自然声音（翻书、咳嗽、
             * 打字等），声音传感器捕捉这些信号来维持灯光。
             * 只有连续 AUTO_OFF_TIMEOUT_MS（5 秒）既无 PIR 也无声音，
             * 才判定"人已离开"并关灯。
             * ======================================================== */

            /*
             * 超时关灯判定：
             * 若距离上一次活动已经超过 AUTO_OFF_TIMEOUT_MS（5 秒），
             * 说明房间内连续指定时间没有任何人体活动迹象 → 关灯。
             */
            if ((last_activity_tick != 0U) &&
                ((HAL_GetTick() - last_activity_tick) > AUTO_OFF_TIMEOUT_MS))
            {
                /* 超时：关灯，清零自动模式内部状态 */
                MOSFET_Update(0);
                auto_light_on = 0U;
                last_activity_tick = 0U;
            }
            else
            {
                /*
                 * 未超时：人在房间里（或刚离开不到 5 秒）。
                 * 继续执行 LDR 环境光自动调光。
                 */

                /* 读取当前环境光 ADC 值（10 次采样取平均） */
                uint16_t adc_value = LDR_ReadAverage();

                /*
                 * 滞回比较（Hysteresis）：
                 * 仅当 ADC 变化量超过 LDR_HYSTERESIS_THRESHOLD(50) 时才更新 PWM。
                 * 避免 ADC 采样噪声导致的灯光微小闪烁，使亮度调节更平滑稳定。
                 */
                uint16_t delta = (adc_value > last_adc)
                                 ? (adc_value - last_adc)
                                 : (last_adc - adc_value);

                if (delta > LDR_HYSTERESIS_THRESHOLD)
                {
                    /* 环境光变化显著 → 更新 PWM 输出 */
                    last_adc = adc_value;
                    last_pwm = LDR_ToPWM(adc_value);
                    MOSFET_Update((int)last_pwm);
                }
                else
                {
                    /* 环境光变化不大 → 维持上一次的 PWM 输出，避免闪烁 */
                    MOSFET_Update((int)last_pwm);
                }
            }
        }
        break;
    }

    /*
     * 主循环延时 50 毫秒（20Hz 轮询频率）。
     *   - 对人体检测（PIR 响应约 100~300ms）足够快
     *   - 能可靠捕获持续时间 ≥100ms 的声音事件（拍手、说话）
     *   - PWM 更新频率 20Hz，远超人眼闪烁感知阈值
     */
    HAL_Delay(50);
  /* USER CODE END 3 */
  }
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

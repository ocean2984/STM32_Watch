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
#include "i2c.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"
#include "oled.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpu6050.h"
#include "max30102.h"
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
float temp;

RTC_TimeTypeDef getTime;
RTC_DateTypeDef getDate;

char timebuf[20];
char datebuf[20];
//MPU6050
int16_t ax,ay,az;
int step = 0;
int flag = 0;
uint32_t last_step_time = 0;
int last_acc = 0;
//MAX30102
uint32_t red=0,ir=0;

uint32_t tick_fast = 0;   // 用于传感器高频采样
uint32_t tick_slow = 0;   // 用于显示和温度低频更新
uint8_t  heart_rate = 0; // 心率初始值
uint8_t  spo2 = 0;       // 血氧初始值
uint32_t last_max30102_tick = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

GPIO_PinState key_state;  // 存储按键状态
 uint8_t led_flag = 0;     // LED状态标志
 // 延时函数（消抖用，HAL库无自带短延时）
 void delay_ms(uint32_t ms) {
   HAL_Delay(ms);
 }
	 
 void Calculate_HeartRate_SPO2(uint32_t red_val, uint32_t ir_val);
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
  MX_I2C1_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	DS18B20_Init();
	
	OLED_Clear();
	
	MPU6050_Init();
	
	MAX30102_Init();
	//====设置时间====
    RTC_TimeTypeDef sTime;
		RTC_DateTypeDef sDate;

		sTime.Hours = 21;
		sTime.Minutes = 10;
		sTime.Seconds = 30;

		HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BIN);

		sDate.Year = 26;
		sDate.Month = RTC_MONTH_APRIL;
		sDate.Date = 15;

		HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
		//====MPU6050====
		
		//==MAX30102====
/*			
uint8_t id;

id = MAX30102_ReadReg(0xFF);

OLED_ShowString(0,7,"ID:");
OLED_ShowNum(24,7,id,3);
*/

  // 这里放一次性的显示提示
  OLED_ShowString(0,0,"Initializing...");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
/* USER CODE BEGIN WHILE */


    while (1)
  {
    // --- 1. 按键控制 ---
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
        HAL_Delay(10);
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
            led_flag = !led_flag;
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, led_flag?GPIO_PIN_RESET:GPIO_PIN_SET);
            while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET);
        }
    }

    // --- 2. 时间、日期、温度 (每秒更新) ---
    if (HAL_GetTick() - tick_slow >= 1000) {
        tick_slow = HAL_GetTick();
        
        // 顺序：先读Time，后读Date
        HAL_RTC_GetTime(&hrtc, &getTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &getDate, RTC_FORMAT_BIN);
        
        // 显示时间
        sprintf(timebuf, "Time: %02d:%02d:%02d   ", getTime.Hours, getTime.Minutes, getTime.Seconds);
        OLED_ShowString(0, 0, timebuf);

        // 显示日期 (2000 + Year)
        sprintf(datebuf, "Date: 20%02d-%02d-%02d", getDate.Year, getDate.Month, getDate.Date);
        OLED_ShowString(0, 1, datebuf);

        // 显示温度
        temp = DS18B20_GetTemp();
        int a = (int)temp;
        int b = (int)((temp - a) * 10);
        OLED_ShowString(0, 2, "Temp: ");
        OLED_ShowNum(48, 2, a, 2);
        OLED_ShowChar(60, 2, '.');
        OLED_ShowNum(66, 2, b, 1);
    }

    // --- 3. 计步任务 (每100ms) ---
    if (HAL_GetTick() - tick_fast >= 100) {
        tick_fast = HAL_GetTick();
        // 现测试心率血氧功能，不接MPU6050，下面这行注释掉
        // MPU6050_ReadAccel(&ax, &ay, &az); 
        OLED_ShowString(0, 3, "Step:");
        OLED_ShowNum(48, 3, step, 5);
    }

    // --- 4. 心率血氧任务 (每40ms采样一次) ---
    if (HAL_GetTick() - last_max30102_tick >= 40) {
        last_max30102_tick = HAL_GetTick();
        
        MAX30102_ReadFIFO(&red, &ir);
        
        // 调试显示：显示ir值测试硬件是否正常检测工作
				//OLED_ShowNum(0, 7, ir, 6); 

        if (ir < 40000) { // 阈值设为 40000，没按手指时显示 ---
            heart_rate = 0;
            spo2 = 0;
            OLED_ShowString(30, 4, "---  ");
            OLED_ShowString(40, 5, "---  ");
        } else {
            // 有手指，调用算法计算心率血氧
            Calculate_HeartRate_SPO2(red, ir);
            
            OLED_ShowString(0, 4, "HR: ");
            if(heart_rate > 0) OLED_ShowNum(30, 4, heart_rate, 3);
            else OLED_ShowString(30, 4, "CAL"); // 正在计算
            OLED_ShowString(60, 4, "BPM");

            OLED_ShowString(0, 5, "SpO2:");
            if(spo2 > 0) OLED_ShowNum(40, 5, spo2, 3);
            else OLED_ShowString(40, 5, "CAL");
            OLED_ShowString(70, 5, "%");
        }
    }
  
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//计算心率血氧
void Calculate_HeartRate_SPO2(uint32_t red_val, uint32_t ir_val)
{
    static uint32_t last_beat_time = 0;
    static float ir_avg = 0;
    
    if(ir_avg < 1.0) ir_avg = ir_val;
    ir_avg = ir_avg * 0.98 + (float)ir_val * 0.02; // 平滑滤波
    
    // 阈值设为 1.005 (即超过平均值 0.5% 就认为是一次心跳)
    if (ir_val > (uint32_t)(ir_avg * 1.005) && (HAL_GetTick() - last_beat_time > 450)) 
    {
        uint32_t interval = HAL_GetTick() - last_beat_time;
        if (interval < 1500) {
            heart_rate = 60000 / interval;
        }
        last_beat_time = HAL_GetTick();
    }
    
    // 血氧计算
    float ratio = (float)red_val / (float)ir_val;
    if (ratio > 0.5 && ratio < 1.2) {
        spo2 = (uint8_t)(110 - 15 * ratio);
        if (spo2 > 100) spo2 = 100;
    } else {
        spo2 = 98; // 默认值
    }
}

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

#ifdef  USE_FULL_ASSERT
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

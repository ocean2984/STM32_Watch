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

int16_t ax,ay,az;
int step = 0;
int flag = 0;
uint32_t last_step_time = 0;
int last_acc = 0;
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
	//====设置时间====
    RTC_TimeTypeDef sTime;
		RTC_DateTypeDef sDate;

		sTime.Hours = 0;
		sTime.Minutes = 10;
		sTime.Seconds = 30;

		HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BIN);

		sDate.Year = 25;
		sDate.Month = RTC_MONTH_MARCH;
		sDate.Date = 15;

		HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
		//====MPU6050====
		
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		//====key-led=====
		 key_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);

    if(key_state == GPIO_PIN_RESET)
    {
        HAL_Delay(10);

        if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_15)==GPIO_PIN_RESET)
        {
            led_flag = !led_flag;

            HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0,
                led_flag?GPIO_PIN_RESET:GPIO_PIN_SET);
        }
    }

		//====时间===
		HAL_RTC_GetTime(&hrtc,&getTime,RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc,&getDate,RTC_FORMAT_BIN);

		sprintf(timebuf,"%02d:%02d:%02d",
        getTime.Hours,
        getTime.Minutes,
        getTime.Seconds);
		
		OLED_ShowString(0,0,"Time:");
		OLED_ShowString(36,0,timebuf);

		sprintf(datebuf,"%04d-%02d-%02d",
        2001 + getDate.Year,
        getDate.Month,
        getDate.Date);

		OLED_ShowString(0,1,"Date:");
		OLED_ShowString(36,1,datebuf);
		
		//====温度====
    temp = DS18B20_GetTemp();
		OLED_ShowString(0,2,"Temp:");
		
     int a = (int)temp;
    int b = (int)((temp - a) * 10);

    OLED_ShowNum(48,2,a,2);
    OLED_ShowChar(60,2,'.');
    OLED_ShowNum(66,2,b,1);

    HAL_Delay(1000);
		//=====MPU6050=====
		MPU6050_ReadAccel(&ax,&ay,&az);
MPU6050_ReadAccel(&ax,&ay,&az);

int az_show = az;
if(az_show < 0) az_show = -az_show;

//检测波峰
if(az_show > 16500 && flag == 0)
{
    //时间过滤（防止抖动）
    if(HAL_GetTick() - last_step_time > 200)
    {
        step++;
        last_step_time = HAL_GetTick();
    }

    flag = 1;
}

//波谷复位
if(az_show < 14000)
{
    flag = 0;
}
OLED_ShowString(0,3,"Step:");
OLED_ShowNum(48,3,step,5);

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

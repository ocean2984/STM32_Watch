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
#include "dma.h"
#include "i2c.h"
#include "rtc.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ds18b20.h"
#include "oled.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpu6050.h"
#include "max30102.h"
#include "esp8266.h"
#include "wifi.h"
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

//wifi
uint32_t tick_wifi = 0;      // 云端上传计时
uint8_t current_page = 0; // 0:时间, 1:健康, 2:计步

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
  MX_DMA_Init();
  MX_RTC_Init();
  MX_USART3_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
  // 这里放一次性的显示提示
  OLED_ShowString(0,0,"Initializing...");
	
	ESP_Init();
	HAL_Delay(3000); //等 3 秒让 WiFi 模块启动完成并喷完乱码

	DS18B20_Init();
	
	OLED_Clear();
	
	MPU6050_Init();
	
	MAX30102_Init();
	
		// 开机自动执行一次联网对时
if(WiFi_Connect()) {
    Sync_Time_From_NowAPI();
}
		//====MPU6050====
		
		//==MAX30102====
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
/* USER CODE BEGIN WHILE */


    while (1)
  {
		 uint8_t dat;

    //电脑(USART1) -> WiFi(USART3)  检查电脑有没有发指令过来
    if (HAL_UART_Receive(&huart1, &dat, 1, 0) == HAL_OK) 
    {
        HAL_UART_Transmit(&huart3, &dat, 1, 10);
    }
	 
    // --- 1. 按键扫描与逻辑处理 (每 50ms 检测一次，保证灵敏度) ---
    if (HAL_GetTick() - tick_fast >= 50) {
        tick_fast = HAL_GetTick();

        // KEY2 (PA14) -> 右滑 (切换页面)
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_14) == GPIO_PIN_RESET) {
            HAL_Delay(20); // 消抖
            if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_14) == GPIO_PIN_RESET) {
                current_page = (current_page + 1) % 3; // 在3个页面间循环
                OLED_Clear(); 
                while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_14) == GPIO_PIN_RESET); // 等待松开
            }
        }

        // KEY1 (PA15) -> 控制 LED1
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
            HAL_Delay(20);
            if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) {
                led_flag = !led_flag;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, led_flag ? GPIO_PIN_RESET : GPIO_PIN_SET);
                while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET);
            }
        }
        
        // KEY4 (PA12) -> 返回第一页
        if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_RESET) {
            current_page = 0;
            OLED_Clear();
        }
    }

    // --- 2. 传感器采样任务 (心率血氧/计步，高频) ---
    if (HAL_GetTick() - last_max30102_tick >= 40) {
        last_max30102_tick = HAL_GetTick();
        
        // (A) 心率血氧采集
        MAX30102_ReadFIFO(&red, &ir);
        if (ir > 40000) {
            Calculate_HeartRate_SPO2(red, ir);
        } else {
            heart_rate = 0; spo2 = 0;
        }

        // (B) 计步采集 
        // MPU6050_ReadAccel(&ax, &ay, &az); 
        // 简易计步算法逻辑放这里...
    }

    // --- 3. UI 刷新与慢速传感器 (时间/温度，每500ms刷新一次) ---
    if (HAL_GetTick() - tick_slow >= 500) {
        tick_slow = HAL_GetTick();
        
        // 更新公共数据：时间和温度
        HAL_RTC_GetTime(&hrtc, &getTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &getDate, RTC_FORMAT_BIN);
        temp = DS18B20_GetTemp();

        // 根据当前页面显示不同内容
        if (current_page == 0) { // 页面0：时间与温度
            sprintf(timebuf, "Time: %02d:%02d:%02d", getTime.Hours, getTime.Minutes, getTime.Seconds);
            OLED_ShowString(0, 0, timebuf);
            sprintf(datebuf, "Date: 20%02d-%02d-%02d", getDate.Year, getDate.Month, getDate.Date);
            OLED_ShowString(0, 2, datebuf);
            
            int a = (int)temp;
            int b = (int)((temp - a) * 10);
						char temp_str[20];
						sprintf(temp_str, "Temp: %2d.%1d C   ", a, b); 
						OLED_ShowString(0, 4, temp_str);
            //OLED_ShowString(0, 4, "Temp: ");
            //OLED_ShowNum(48, 4, a, 2); OLED_ShowChar(60, 4, '.'); OLED_ShowNum(66, 4, b, 1);
        } 
        else if (current_page == 1) { // 页面1：心率与血氧
            OLED_ShowString(0, 0, "--- Health ---");
            OLED_ShowString(0, 2, "HR: ");
            if(heart_rate > 0) OLED_ShowNum(40, 2, heart_rate, 3);
            else OLED_ShowString(40, 2, "Wait");
            OLED_ShowString(75, 2, "BPM");

            OLED_ShowString(0, 4, "SpO2:");
            if(spo2 > 0) OLED_ShowNum(40, 4, spo2, 3);
            else OLED_ShowString(40, 4, "Wait");
            OLED_ShowString(75, 4, "%");
        }
        else if (current_page == 2) { // 页面2：计步与状态
            OLED_ShowString(0, 0, "--- Sport ---");
            OLED_ShowString(0, 2, "Steps:");
            OLED_ShowNum(50, 2, step, 5);
            OLED_ShowString(0, 4, "WiFi: Connected");
        }
    }

    // --- 4. WiFi 云端上传任务 (每 20 秒上报一次数据) ---
    if (HAL_GetTick() - tick_wifi >= 20000) {
        tick_wifi = HAL_GetTick();
        
        // 调用封装在 wifi_app.c 里的函数上传数据
        // ThingsCloud_Upload(heart_rate, spo2, temp, step);
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

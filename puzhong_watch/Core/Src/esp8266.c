#include "esp8266.h"
#include <string.h>
#include <stdio.h>
#include "usart.h"

char esp_buf[512];
uint16_t esp_pos = 0;
uint8_t esp_data;

// 初始化ESP8266
void ESP_Init(void) {
    memset(esp_buf, 0, sizeof(esp_buf));
    esp_pos = 0;
    HAL_UART_Receive_IT(&huart3, &esp_data, 1);
}

// 清空缓冲区
void ESP_ClearBuf(void) {
    memset(esp_buf, 0, sizeof(esp_buf));
    esp_pos = 0;
}

uint8_t ESP_SendCmd(char *cmd, char *reply, uint16_t timeout) {
    ESP_ClearBuf();
    HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 100);
    HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 10);
    
    uint32_t tick = HAL_GetTick();
    while (HAL_GetTick() - tick < timeout) {
        // 如果这里能打印出 esp_buf，就能看到到底回没回
        if (esp_pos > 0) {
             // 调试用：HAL_UART_Transmit(&huart1, (uint8_t*)esp_buf, esp_pos, 100);
        }

        if (strstr(esp_buf, reply)) {
            return 1;
        }
        HAL_Delay(10);
    }
    return 0;
}

// 串口中断回调函数
extern uint8_t led_flag; // 引用 main.c 里的 LED 状态
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) { 
        if (esp_pos < 511) {
            esp_buf[esp_pos++] = esp_data;
            
            // --- 实时解析小程序下发的指令 ---
            // 假设小程序下发属性 "led_switch": 1 或 0
            if (strstr(esp_buf, "\"led_switch\":1")) {
                led_flag = 1;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET); // 开灯
                ESP_ClearBuf(); // 清空以防重复判断
            } else if (strstr(esp_buf, "\"led_switch\":0")) {
                led_flag = 0;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);   // 关灯
                ESP_ClearBuf();
            }
        } else {
            esp_pos = 0;
        }
        HAL_UART_Receive_IT(&huart3, &esp_data, 1);
    }
}

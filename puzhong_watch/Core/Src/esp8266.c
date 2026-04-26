#include "esp8266.h"
#include <string.h>
#include <stdio.h>
#include "usart.h"

char esp_buf[512];
uint16_t esp_pos = 0;
uint8_t esp_data;

void ESP_Init(void) {
    memset(esp_buf, 0, 512);
    esp_pos = 0;
    // 开启中断接收
    HAL_UART_Receive_IT(&huart3, &esp_data, 1);
}

void ESP_ClearBuf(void) {
    memset(esp_buf, 0, 512);
    esp_pos = 0;
}

uint8_t ESP_SendCmd(char *cmd, char *reply, uint16_t timeout) {
    ESP_ClearBuf();
    char temp[256]; 
    sprintf(temp, "%s\r\n", cmd);
    HAL_UART_Transmit(&huart3, (uint8_t*)temp, strlen(temp), 100);
    
    uint32_t tick = HAL_GetTick();
    while (HAL_GetTick() - tick < timeout) {
        if (strstr(esp_buf, reply)) {
            // 匹配成功，把收到的全部内容发给电脑看看
            HAL_UART_Transmit(&huart1, (uint8_t*)esp_buf, strlen(esp_buf), 100);
            return 1;
        }
        HAL_Delay(10);
    }
    // 匹配失败，也发给电脑看看回了什么乱七八糟的
    HAL_UART_Transmit(&huart1, (uint8_t*)"Timeout Content: ", 17, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)esp_buf, strlen(esp_buf), 100);
    return 0;
}
// 串口中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART3) { 
        if (esp_pos < 511) {
            esp_buf[esp_pos++] = esp_data;
        }
        // 仅仅开启下一次接收，不做任何耗时操作
        HAL_UART_Receive_IT(&huart3, &esp_data, 1);
    }
}

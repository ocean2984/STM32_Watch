#ifndef __ESP8266_H
#define __ESP8266_H
#include "main.h"

// 基础AT指令函数
uint8_t ESP_SendCmd(char *cmd, char *reply, uint16_t timeout);
void ESP_Init(void);
void ESP_ClearBuf(void);

extern char esp_buf[512]; // 接收缓冲区
#endif

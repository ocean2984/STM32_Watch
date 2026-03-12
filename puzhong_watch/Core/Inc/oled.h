#ifndef __OLED_H
#define __OLED_H

#include "stm32f1xx_hal.h"

#define OLED_ADDR (0x3C<<1)   // HAL Òª×óÒÆ1Î»

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowString(uint8_t x, uint8_t y, char *chr);

void OLED_ShowChinese(uint8_t x, uint8_t y, const uint8_t *font);
void OLED_ShowTemperatureExample(void);

#endif
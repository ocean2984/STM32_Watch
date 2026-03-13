#ifndef __OLED_H
#define __OLED_H

#include "main.h"

#define OLED_ADDRESS 0x3C << 1

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x,uint8_t y);

void OLED_ShowChar(uint8_t x,uint8_t y,char chr);
void OLED_ShowString(uint8_t x,uint8_t y,char *str);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len);
#endif

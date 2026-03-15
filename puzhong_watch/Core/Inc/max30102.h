#ifndef __MAX30102_H
#define __MAX30102_H

#include "main.h"

#define MAX30102_ADDRESS 0x57

void MAX30102_Init(void);
void MAX30102_ReadFIFO(uint32_t *red_led, uint32_t *ir_led);
uint8_t MAX30102_ReadReg(uint8_t reg);

#endif

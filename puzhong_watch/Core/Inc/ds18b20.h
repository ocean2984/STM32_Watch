#ifndef __DS18B20_H
#define __DS18B20_H

#include "main.h"

#define DS18B20_PORT GPIOA
#define DS18B20_PIN  GPIO_PIN_11

uint8_t DS18B20_Init(void);
float DS18B20_GetTemp(void);

#endif

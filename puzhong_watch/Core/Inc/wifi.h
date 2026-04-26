#ifndef __WIFI_APP_H
#define __WIFI_APP_H

#include "main.h"
#include <stdio.h>
#include <string.h>
#include "oled.h"

// 声明在 wifi.c 里写的函数
uint8_t WiFi_Connect(void);
void Sync_Time_From_NowAPI(void);
void ThingsCloud_Upload(int hr, int spo2, float temp, int steps);
void Refresh_OLED(uint8_t page);

#endif

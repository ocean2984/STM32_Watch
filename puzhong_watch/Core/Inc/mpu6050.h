#ifndef __MPU6050_H
#define __MPU6050_H

#include "i2c.h"

void MPU6050_Init(void);

void MPU6050_ReadAccel(int16_t *ax,int16_t *ay,int16_t *az);

#endif

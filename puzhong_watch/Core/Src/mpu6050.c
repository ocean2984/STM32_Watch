#include "mpu6050.h"

#define MPU_ADDR (0x68<<1)

//初始化MPU6050
void MPU6050_Init(void)
{
    uint8_t data;

    // 退出睡眠
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1,0xD0,0x6B,1,&data,1,100);

    // 设置加速度量程 ±2g
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1,0xD0,0x1C,1,&data,1,100);

    // 设置陀螺仪量程
    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1,0xD0,0x1B,1,&data,1,100);
}

//读取三轴加速度
void MPU6050_ReadAccel(int16_t *ax,int16_t *ay,int16_t *az)
{
    uint8_t buf[6];

    HAL_I2C_Mem_Read(&hi2c1,
                     0xD0,
                     0x3B,
                     1,
                     buf,
                     6,
                     100);

    *ax = (buf[0]<<8)|buf[1];
    *ay = (buf[2]<<8)|buf[3];
    *az = (buf[4]<<8)|buf[5];
}

#include "ds18b20.h"

/* IO方向控制 */

static void DS18B20_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

static void DS18B20_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DS18B20_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

/* 复位 */

static void DS18B20_Reset(void)
{
    DS18B20_OUT();

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
}

/* 初始化 */

void DS18B20_Init(void)
{
    DS18B20_Reset();
}

/* 读取温度（测试版） */

float DS18B20_GetTemp(void)
{
    DS18B20_Reset();

    /* 先返回测试温度 */

    return 25.0;
}

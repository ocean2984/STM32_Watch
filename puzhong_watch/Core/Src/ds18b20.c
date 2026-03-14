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

// 微秒延时
void HAL_Delay_us(uint32_t us)
{
    uint32_t delay = us * (SystemCoreClock / 1000000 / 4);
    while(delay--);
}
// ===================== 初始化 =====================
uint8_t DS18B20_Init(void)
{
    uint8_t ack = 0;

    DS18B20_OUT();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    HAL_Delay_us(480);   // 拉低480us

    DS18B20_IN();
    HAL_Delay_us(60);    // 等待应答

    // 读到低电平 = DS18B20存在
    if(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == 0) ack = 1;

    HAL_Delay_us(420);
    return ack;
}

// ===================== 写字节 =====================
void DS18B20_WriteByte(uint8_t dat)
{
    uint8_t i;
    DS18B20_OUT();

    for(i=0; i<8; i++)
    {
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        HAL_Delay_us(2);

        if(dat & 0x01)
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);

        HAL_Delay_us(60);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
        dat >>= 1;
    }
}

// ===================== 读字节 =====================
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, dat = 0;
    for(i=0; i<8; i++)
    {
        DS18B20_OUT();
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        HAL_Delay_us(2);

        DS18B20_IN();
         if(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET)
        {
            dat |= (1 << i);
        }

        HAL_Delay_us(60);
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
    }
    return dat;
}

// ===================== 读温度 =====================
float DS18B20_GetTemp(void)
{
    uint8_t L, M;
    int16_t temp;

    DS18B20_Init();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);  // 开始转换
	
    DS18B20_Init();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);  // 读数据

    L = DS18B20_ReadByte();
    M = DS18B20_ReadByte();

    temp = (M << 8) | L;
    return temp * 0.0625f;
}

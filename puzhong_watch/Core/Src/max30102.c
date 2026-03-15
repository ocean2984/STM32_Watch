#include "max30102.h"

extern I2C_HandleTypeDef hi2c1;

/* Ð´¼Ä´æÆ÷ */
void MAX30102_WriteReg(uint8_t reg, uint8_t data)
{
    HAL_I2C_Mem_Write(&hi2c1,
                      MAX30102_ADDRESS,
                      reg,
                      I2C_MEMADD_SIZE_8BIT,
                      &data,
                      1,
                      100);
}

/* ¶Á¼Ä´æÆ÷ */
uint8_t MAX30102_ReadReg(uint8_t reg)
{
    uint8_t data;

    HAL_I2C_Master_Transmit(&hi2c1, 0x57<<1, &reg, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, 0x57<<1, &data, 1, 100);

    return data;
}

/* MAX30102³õÊ¼»¯ */
void MAX30102_Init(void)
{
    // ¸´Î»
    MAX30102_WriteReg(0x09,0x40);
    HAL_Delay(100);

    // FIFOÅäÖÃ
    MAX30102_WriteReg(0x08,0x0F);

    // Ä£Ê½£ººì¹â+ºìÍâ
    MAX30102_WriteReg(0x09,0x03);

    // SpO2ÅäÖÃ
    // ADC·¶Î§4096nA ²ÉÑùÂÊ100Hz LEDÂö¿í411us
    MAX30102_WriteReg(0x0A,0x23);

    // LEDµçÁ÷
    MAX30102_WriteReg(0x0C,0x1F);
    MAX30102_WriteReg(0x0D,0x1F);

    // FIFOÖ¸ÕëÇåÁã
    MAX30102_WriteReg(0x04,0x00);
    MAX30102_WriteReg(0x05,0x00);
    MAX30102_WriteReg(0x06,0x00);
}

/* ¶ÁÈ¡FIFO */
void MAX30102_ReadFIFO(uint32_t *red_led, uint32_t *ir_led)
{
    uint8_t data[6];

    HAL_I2C_Mem_Read(
        &hi2c1,
        0xAE,              //MAX30102µØÖ·
        0x07,              //FIFO_DATA¼Ä´æÆ÷
        I2C_MEMADD_SIZE_8BIT,
        data,
        6,
        100
    );

    
    *ir_led  = ((uint32_t)data[3]<<16 | (uint32_t)data[4]<<8 | data[5]) & 0x3FFFFF;
		*red_led = ((uint32_t)data[0]<<16 | (uint32_t)data[1]<<8 | data[2]) & 0x3FFFFF;
}


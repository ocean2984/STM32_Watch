#include "oled.h"
#include "i2c.h"
#include "oledfont.h"

#define OLED_ADDR 0x78

/* 写命令 */
void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t data[2];

    data[0]=0x00;
    data[1]=cmd;

    HAL_I2C_Master_Transmit(&hi2c1,OLED_ADDR,data,2,100);
}

/* 写数据 */
void OLED_WriteData(uint8_t data)
{
    uint8_t temp[2];

    temp[0]=0x40;
    temp[1]=data;

    HAL_I2C_Master_Transmit(&hi2c1,OLED_ADDR,temp,2,100);
}

/* 设置光标位置 */
void OLED_SetCursor(uint8_t x,uint8_t y)
{
    OLED_WriteCmd(0xb0+y);
    OLED_WriteCmd(((x&0xf0)>>4)|0x10);
    OLED_WriteCmd((x&0x0f)|0x01);
}

/* 清屏 */
void OLED_Clear(void)
{
    for(uint8_t i=0;i<8;i++)
    {
        OLED_WriteCmd(0xb0+i);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);

        for(uint8_t n=0;n<128;n++)
        {
            OLED_WriteData(0);
        }
    }
}

/* 初始化OLED */
void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0xb0);
    OLED_WriteCmd(0xc8);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xff);
    OLED_WriteCmd(0xa1);
    OLED_WriteCmd(0xa6);
    OLED_WriteCmd(0xa8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xa4);
    OLED_WriteCmd(0xd3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xd5);
    OLED_WriteCmd(0xf0);
    OLED_WriteCmd(0xd9);
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xda);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xdb);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8d);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xaf);

    OLED_Clear();
}

/* 显示一个字符 */
void OLED_ShowChar(uint8_t x,uint8_t y,char chr)
{
    uint8_t c = chr - ' ';

    OLED_SetCursor(x,y);

    for(uint8_t i=0;i<6;i++)
    {
        OLED_WriteData(F6x8[c][i]);
    }
}

/* 显示字符串 */
void OLED_ShowString(uint8_t x,uint8_t y,char *str)
{
    while(*str)
    {
        OLED_ShowChar(x,y,*str);
        x+=6;

        if(x>122)
        {
            x=0;
            y++;
        }

        str++;
    }
}

/* 显示数字 */
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len)
{
    uint8_t temp;

    for(uint8_t i=0;i<len;i++)
    {
        temp = num % 10;

        OLED_ShowChar(x+6*(len-i-1),y,temp+'0');

        num/=10;
    }
}

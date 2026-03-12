#include "main.h"
#include "oled.h"

extern I2C_HandleTypeDef hi2c1;

static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t data[2];
    data[0] = 0x00;
    data[1] = cmd;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, 2, 100);
}

static void OLED_WriteData(uint8_t data)
{
    uint8_t buf[2];
    buf[0] = 0x40;
    buf[1] = data;
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, buf, 2, 100);
}

void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0xB0);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xFF);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xA4);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xAF);
}

void OLED_Clear(void)
{
    for(uint8_t page=0;page<8;page++)
    {
        OLED_WriteCmd(0xB0+page);
        OLED_WriteCmd(0x00);
        OLED_WriteCmd(0x10);
        for(uint8_t col=0;col<128;col++)
        {
            OLED_WriteData(0xFF);
        }
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *chr)
{
    OLED_WriteCmd(0xB0+y);
    OLED_WriteCmd(((x&0xF0)>>4)|0x10);
    OLED_WriteCmd((x&0x0F)|0x00);

    while(*chr)
    {
        for(uint8_t i=0;i<6;i++)
        {
            OLED_WriteData(0x00);//0x00全灭，0xFF全亮
        }
        chr++;
    }
}

// -----------------------------
// oled_chinese.c
// -----------------------------

#include "oled.h"  // OLED 驱动函数头文件
#include <stdint.h> // uint8_t 类型

// -----------------------------
// 1. 汉字点阵数组（16x16, 32 字节/字）
// -----------------------------
const uint8_t chinese[][32] = {
    // "温"
    {0x08,0x20,0x06,0x20,0x40,0x7E,0x31,0x80,
     0x00,0x02,0x00,0x7E,0x7F,0x42,0x49,0x42,
     0x49,0x7E,0x49,0x42,0x49,0x7E,0x49,0x42,
     0x7F,0x42,0x00,0x7E,0x00,0x02,0x00,0x00},
    // "度"
    {0x00,0x02,0x00,0x0C,0x3F,0xF1,0x24,0x01,
     0x24,0x21,0x24,0x32,0x3F,0xAA,0xA4,0xA4,
     0x64,0xA4,0x24,0xA4,0x3F,0xAA,0x24,0x32,
     0x24,0x01,0x24,0x01,0x20,0x01,0x00,0x00},
    // "："
    {0x00,0x00,0x00,0x00,0x00,0x6C,0x00,0x6C,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

// -----------------------------
// 2. OLED 显示单个汉字函数
// x: 横向起始列 (0~127)
// y: 页号 (0~7, 每页 8 行)
// font: 16x16 点阵数组
// -----------------------------
void OLED_SetCursor(uint8_t x, uint8_t y)
{
    OLED_WriteCmd(0xB0 + y);                  // 设置页地址（y）
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10); // 设置列高四位
    OLED_WriteCmd((x & 0x0F) | 0x00);        // 设置列低四位
}

void OLED_ShowChinese(uint8_t x, uint8_t y, const uint8_t *font)
{
    // 每个字占 16 列，纵向两页
    for(uint8_t page = 0; page < 2; page++)  // 0~1 页
    {
        OLED_SetCursor(x, y + page); // 设置页起始位置

        for(uint8_t col = 0; col < 16; col++) // 每页 16 列
        {
            // OLED_WriteData 发送字节到 OLED 显存
            OLED_WriteData(font[page*16 + col]);
        }
    }
}

// -----------------------------
// 3. 调用示例：显示 "温度：13"
// -----------------------------
void OLED_ShowTemperatureExample(void)
{
    // 显示 "温"
    OLED_ShowChinese(0, 0, chinese[0]);

    // 显示 "度"
    OLED_ShowChinese(16, 0, chinese[1]);

    // 显示 "："
    OLED_ShowChinese(32, 0, chinese[2]);

    // 数字 "13" 可以用之前的 OLED_ShowString 显示 ASCII
    OLED_ShowString(48, 0, "13");
}
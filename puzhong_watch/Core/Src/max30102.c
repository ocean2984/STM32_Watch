#include "max30102.h"

extern I2C_HandleTypeDef hi2c1;

/* 写寄存器 */
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

/* 读寄存器 */
uint8_t MAX30102_ReadReg(uint8_t reg)
{
    uint8_t data;

    HAL_I2C_Master_Transmit(&hi2c1, 0x57<<1, &reg, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1, 0x57<<1, &data, 1, 100);

    return data;
}

//MAX30102初始化
/*void MAX30102_Init(void)
{
    MAX30102_WriteReg(0x09, 0x40); // 1. 强制复位
    HAL_Delay(100);

    MAX30102_WriteReg(0x01, 0xC0); // 2. 开启中断：数据准备就绪 + FIFO满
    MAX30102_WriteReg(0x04, 0x00); // 3. 清空指针
    MAX30102_WriteReg(0x05, 0x00);
    MAX30102_WriteReg(0x06, 0x00);

    MAX30102_WriteReg(0x08, 0x10); // 4. FIFO配置：无平均，开启滚动覆盖
    MAX30102_WriteReg(0x09, 0x03); // 5. 模式配置：SpO2模式
    MAX30102_WriteReg(0x0A, 0x27); // 6. SpO2配置：100Hz, 411us, 18bit
    MAX30102_WriteReg(0x0C, 0x32); // 7. LED1 (红光) 电流：约10mA
    MAX30102_WriteReg(0x0D, 0x32); // 8. LED2 (红外) 电流：约10mA
}
*/
void MAX30102_Init(void)
{
    // 1. 软件复位
    MAX30102_WriteReg(0x09, 0x40);
    HAL_Delay(500); // 增加复位等待时间，确保芯片反应过来

    // 2. 检查通信是否正常（可选：读一下ID寄存器0xFF，看是不是0x15）

    // 3. 中断配置：清零
    MAX30102_WriteReg(0x02, 0x00);
    MAX30102_WriteReg(0x03, 0x00);

    // 4. FIFO配置
    MAX30102_WriteReg(0x04, 0x00); // 写指针
    MAX30102_WriteReg(0x05, 0x00); // 溢出计数
    MAX30102_WriteReg(0x06, 0x00); // 读指针
    MAX30102_WriteReg(0x08, 0x50); // 平均4采样，开启滚动覆盖

    // 5. 模式配置：0x03 是 SpO2 模式（红光+红外均开启）
    // 如果这里设置成 0x02，就只有红外光，肉眼看不见
    MAX30102_WriteReg(0x09, 0x03); 

    // 6. SpO2参数
    MAX30102_WriteReg(0x0A, 0x27); // 100Hz, 411us

    // 7. LED 电流控制（这是让灯亮起来的关键！）
    // 0x24 对应约 7mA，0x32 对应约 10mA。
    MAX30102_WriteReg(0x0C, 0x32); // LED1 (红光) 电流
    MAX30102_WriteReg(0x0D, 0x32); // LED2 (红外) 电流
    
    // 8. 临近检测（关闭）
    MAX30102_WriteReg(0x10, 0x00); 
}

/* 读取FIFO */
void MAX30102_ReadFIFO(uint32_t *red_led, uint32_t *ir_led)
{
    uint8_t data[6];

    // 从 0x07 寄存器读取 6 个字节
    HAL_I2C_Mem_Read(&hi2c1, 0xAE, 0x07, I2C_MEMADD_SIZE_8BIT, data, 6, 100);

    // 合并数据
    *red_led = ((uint32_t)data[0] << 16 | (uint32_t)data[1] << 8 | data[2]) & 0x03FFFF;
    *ir_led  = ((uint32_t)data[3] << 16 | (uint32_t)data[4] << 8 | data[5]) & 0x03FFFF;

    // --- 重点：每读一次，手动重置一次指针，防止仓库卡死 ---
    // 虽然开了 Rollover，但手动清零是最稳妥的
    MAX30102_WriteReg(0x04, 0x00); 
    MAX30102_WriteReg(0x05, 0x00);
    MAX30102_WriteReg(0x06, 0x00);
}


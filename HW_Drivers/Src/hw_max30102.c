/**
 * @file    hw_max30102.c
 * @brief   MAX30102 心率/血氧传感器驱动实现
 */

#include "hw_max30102.h"

/* ========================== 底层 I2C 通信 ========================== */

void MAX30102_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    HAL_I2C_Master_Transmit(&MAX30102_I2C, MAX30102_I2C_ADDR, buf, 2, 100);
}

uint8_t MAX30102_ReadReg(uint8_t reg)
{
    uint8_t value = 0;
    HAL_I2C_Master_Transmit(&MAX30102_I2C, MAX30102_I2C_ADDR, &reg, 1, 100);
    HAL_I2C_Master_Receive(&MAX30102_I2C, MAX30102_I2C_ADDR, &value, 1, 100);
    return value;
}

void MAX30102_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
    HAL_I2C_Master_Transmit(&MAX30102_I2C, MAX30102_I2C_ADDR, &reg, 1, 100);
    HAL_I2C_Master_Receive(&MAX30102_I2C, MAX30102_I2C_ADDR, data, len, 100);
}

/* ========================== 初始化 ========================== */

void MAX30102_Init(void)
{
    /* 软件复位 */
    MAX30102_Reset();
    HAL_Delay(10);

    /* 复位后默认寄存器值：
     *   MODE_CONFIG = 0x00 (Shutdown)
     *   SPO2_CONFIG = 0x00
     *   FIFO_CONFIG = 0x00
     *   需重新配置
     */

    /* FIFO 配置: 每样本 3 字节 IR + 3 字节 Red = 6 字节, Rolling 模式 */
    MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, 0x00);

    /* SpO2 配置: 100Hz, 411us 脉宽, 18-bit ADC */
    MAX30102_SetSampleRate(MAX30102_SR_100);
    MAX30102_SetPulseWidth(MAX30102_PW_411US);

    /* LED 电流: Red 12mA, IR 12mA (低功耗优化) */
    MAX30102_SetLEDCurrent(MAX30102_LED_CURRENT_12MA, MAX30102_LED_CURRENT_12MA);

    /* 清除 FIFO 指针和中断 */
    MAX30102_ClearFIFO();
    MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00);
}

void MAX30102_Reset(void)
{
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, 0x40);
}

uint8_t MAX30102_GetPartID(void)
{
    return MAX30102_ReadReg(MAX30102_REG_PART_ID);   /* 应为 0x15 */
}

/* ========================== 工作模式配置 ========================== */

void MAX30102_SetMode(MAX30102_Mode_t mode)
{
    MAX30102_WriteReg(MAX30102_REG_MODE_CONFIG, (uint8_t)mode);
}

void MAX30102_SetSampleRate(MAX30102_SampleRate_t rate)
{
    uint8_t reg = MAX30102_ReadReg(MAX30102_REG_SPO2_CONFIG);
    reg &= ~0x1C;                      /* 清除 bit[4:2] */
    reg |= ((uint8_t)rate << 2);
    MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, reg);
}

void MAX30102_SetPulseWidth(MAX30102_PulseWidth_t pw)
{
    uint8_t reg = MAX30102_ReadReg(MAX30102_REG_SPO2_CONFIG);
    reg &= ~0x03;                      /* 清除 bit[1:0] */
    reg |= (uint8_t)pw;
    MAX30102_WriteReg(MAX30102_REG_SPO2_CONFIG, reg);
}

void MAX30102_SetLEDCurrent(MAX30102_LEDCurrent_t red, MAX30102_LEDCurrent_t ir)
{
    MAX30102_WriteReg(MAX30102_REG_LED1_PA, (uint8_t)red);
    MAX30102_WriteReg(MAX30102_REG_LED2_PA, (uint8_t)ir);
}

/* ========================== FIFO 操作 ========================== */

void MAX30102_SetFIFOAlmostFull(uint8_t samples)
{
    uint8_t reg = MAX30102_ReadReg(MAX30102_REG_FIFO_CONFIG);
    reg &= ~0x0F;
    reg |= (samples & 0x0F);
    MAX30102_WriteReg(MAX30102_REG_FIFO_CONFIG, reg);
}

uint8_t MAX30102_GetFIFOSampleCount(void)
{
    uint8_t wr_ptr = MAX30102_ReadReg(MAX30102_REG_FIFO_WR_PTR);
    uint8_t rd_ptr = MAX30102_ReadReg(MAX30102_REG_FIFO_RD_PTR);

    if (wr_ptr >= rd_ptr)
        return (wr_ptr - rd_ptr);
    else
        return (32 - rd_ptr + wr_ptr);
}

/**
 * @brief  读取单个 FIFO 样本 (IR + Red)
 * @note   每样本 6 字节: [IR[17:10], IR[9:2], IR[1:0]xx_xxxx, Red[17:10], Red[9:2], Red[1:0]xx_xxxx]
 */
void MAX30102_ReadFIFO(uint32_t *ir, uint32_t *red)
{
    uint8_t data[6];
    MAX30102_ReadMulti(MAX30102_REG_FIFO_DATA, data, 6);

    /* 解析 18-bit IR */
    *ir  = ((uint32_t)data[0] << 16) & 0x00030000;
    *ir |= ((uint32_t)data[1] << 8);
    *ir |= data[2] & 0xFC;
    *ir >>= 2;

    /* 解析 18-bit Red */
    *red  = ((uint32_t)data[3] << 16) & 0x00030000;
    *red |= ((uint32_t)data[4] << 8);
    *red |= data[5] & 0xFC;
    *red >>= 2;
}

void MAX30102_ClearFIFO(void)
{
    MAX30102_WriteReg(MAX30102_REG_FIFO_RD_PTR, 0x00);
    MAX30102_WriteReg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    MAX30102_WriteReg(MAX30102_REG_OVF_COUNTER, 0x00);
}

/* ========================== 中断控制 ========================== */

void MAX30102_EnableInterrupt(uint8_t int_flags)
{
    MAX30102_WriteReg(MAX30102_REG_INT_ENABLE1, int_flags);
}

void MAX30102_DisableInterrupt(uint8_t int_flags)
{
    uint8_t reg = MAX30102_ReadReg(MAX30102_REG_INT_ENABLE1);
    reg &= ~int_flags;
    MAX30102_WriteReg(MAX30102_REG_INT_ENABLE1, reg);
}

/* ========================== 温度读取 ========================== */

float MAX30102_ReadTemperature(void)
{
    /* 使能温度传感器 */
    MAX30102_WriteReg(MAX30102_REG_TEMP_CONFIG, 0x01);

    /* 等待转换完成 (最多 30ms) */
    HAL_Delay(30);

    int8_t temp_int  = (int8_t)MAX30102_ReadReg(MAX30102_REG_TEMP_INTR);
    uint8_t temp_frac = MAX30102_ReadReg(MAX30102_REG_TEMP_FRAC);

    return (float)temp_int + (float)temp_frac * 0.0625f;
}

/**
 * @file    hw_adxl345.c
 * @brief   ADXL345 三轴加速度计驱动实现
 */

#include "hw_adxl345.h"

/* ========================== 片选控制 ========================== */

void ADXL345_CS_Low(void)
{
    HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_RESET);
}

void ADXL345_CS_High(void)
{
    HAL_GPIO_WritePin(ADXL345_CS_PORT, ADXL345_CS_PIN, GPIO_PIN_SET);
}

/* ========================== 底层 SPI 通信 ========================== */

void ADXL345_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2];

    ADXL345_CS_Low();
    tx_data[0] = reg & ~ADXL345_SPI_READ;      /* 写标志 */
    tx_data[1] = value;
    HAL_SPI_Transmit(&ADXL345_SPI, tx_data, 2, 50);
    ADXL345_CS_High();
}

uint8_t ADXL345_ReadReg(uint8_t reg)
{
    uint8_t tx_data, rx_data;

    ADXL345_CS_Low();
    tx_data = reg | ADXL345_SPI_READ;           /* 读标志 */
    HAL_SPI_Transmit(&ADXL345_SPI, &tx_data, 1, 50);
    HAL_SPI_Receive(&ADXL345_SPI, &rx_data, 1, 50);
    ADXL345_CS_High();

    return rx_data;
}

void ADXL345_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
    ADXL345_CS_Low();
    uint8_t tx_addr = reg | ADXL345_SPI_READ | ADXL345_SPI_MB;
    HAL_SPI_Transmit(&ADXL345_SPI, &tx_addr, 1, 50);
    HAL_SPI_Receive(&ADXL345_SPI, data, len, 50);
    ADXL345_CS_High();
}

/* ========================== 初始化 ========================== */

void ADXL345_Init(void)
{
    ADXL345_CS_High();

    /* 检查设备 ID */
    uint8_t devid = ADXL345_GetDevID();

    /* 数据格式: ±16g, 全分辨率, 右对齐 */
    ADXL345_WriteReg(ADXL345_REG_DATA_FORMAT, 0x0B);

    /* 采样率: 200Hz */
    ADXL345_WriteReg(ADXL345_REG_BW_RATE, ADXL345_RATE_200HZ);

    /* FIFO: Bypass 模式 */
    ADXL345_WriteReg(ADXL345_REG_FIFO_CTL, 0x00);

    /* 自由落体检测配置 (用于跌倒辅助检测) */
    ADXL345_WriteReg(ADXL345_REG_THRESH_FF, 0x05);    /* 约 312mg 阈值   */
    ADXL345_WriteReg(ADXL345_REG_TIME_FF, 0x14);      /* 20ms 持续时间   */

    /* 活动检测配置 */
    ADXL345_WriteReg(ADXL345_REG_THRESH_ACT, 0x20);   /* 约 2g 阈值     */
    ADXL345_WriteReg(ADXL345_REG_ACT_INACT_CTL, 0xFF);/* 所有轴参与     */

    /* 启动测量模式 */
    ADXL345_WriteReg(ADXL345_REG_POWER_CTL, 0x08);    /* Measure 模式   */
}

uint8_t ADXL345_GetDevID(void)
{
    return ADXL345_ReadReg(ADXL345_REG_DEVID);        /* 应为 0xE5 */
}

/* ========================== 配置 ========================== */

void ADXL345_SetRange(ADXL345_Range_t range)
{
    uint8_t reg = ADXL345_ReadReg(ADXL345_REG_DATA_FORMAT);
    reg &= ~0x03;
    reg |= (range & 0x03);
    reg |= (1 << 3);  /* FULL_RES 位：保持全分辨率 */
    ADXL345_WriteReg(ADXL345_REG_DATA_FORMAT, reg);
}

void ADXL345_SetRate(ADXL345_Rate_t rate)
{
    uint8_t reg = ADXL345_ReadReg(ADXL345_REG_BW_RATE);
    reg &= ~0x0F;
    reg |= (rate & 0x0F);
    ADXL345_WriteReg(ADXL345_REG_BW_RATE, reg);
}

void ADXL345_EnableInterrupts(void)
{
    /* 使能: Data Ready + Free Fall + Activity */
    ADXL345_WriteReg(ADXL345_REG_INT_ENABLE,
                     ADXL345_INT_DATA_READY |
                     ADXL345_INT_FREE_FALL   |
                     ADXL345_INT_ACTIVITY);

    /* 映射到 INT1 */
    ADXL345_WriteReg(ADXL345_REG_INT_MAP, 0x00);  /* 全部到 INT1 */
}

/* ========================== 数据读取 ========================== */

void ADXL345_ReadAccel(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t data[6];

    ADXL345_ReadMulti(ADXL345_REG_DATAX0, data, 6);

    /* 合并高低字节 */
    *x = (int16_t)((data[1] << 8) | data[0]);
    *y = (int16_t)((data[3] << 8) | data[2]);
    *z = (int16_t)((data[5] << 8) | data[4]);
}

uint8_t ADXL345_GetIntSource(void)
{
    return ADXL345_ReadReg(ADXL345_REG_INT_SOURCE);
}

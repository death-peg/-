/**
 * @file    hw_adxl345.h
 * @brief   ADXL345 三轴加速度计驱动 - 头文件
 *
 * 通信接口: SPI1 (5MHz, Mode 3)
 * 用途:     撞击检测、跌倒检测、运动姿态
 */

#ifndef __HW_ADXL345_H
#define __HW_ADXL345_H

#include "main.h"
#include <stdint.h>

/* ========================== ADXL345 寄存器地址 ========================== */
#define ADXL345_REG_DEVID          0x00    /* 设备ID (应为 0xE5)     */
#define ADXL345_REG_THRESH_TAP     0x1D    /* 单击阈值               */
#define ADXL345_REG_OFSX           0x1E
#define ADXL345_REG_OFSY           0x1F
#define ADXL345_REG_OFSZ           0x20
#define ADXL345_REG_DUR            0x21    /* 单击持续时间           */
#define ADXL345_REG_LATENT         0x22    /* 双击延迟               */
#define ADXL345_REG_WINDOW         0x23    /* 双击窗口               */
#define ADXL345_REG_THRESH_ACT     0x24    /* 活动阈值               */
#define ADXL345_REG_THRESH_INACT   0x25    /* 静止阈值               */
#define ADXL345_REG_TIME_INACT     0x26    /* 静止时间               */
#define ADXL345_REG_ACT_INACT_CTL  0x27    /* 活动/静止轴使能        */
#define ADXL345_REG_THRESH_FF      0x28    /* 自由落体阈值           */
#define ADXL345_REG_TIME_FF        0x29    /* 自由落体时间           */
#define ADXL345_REG_TAP_AXES       0x2A    /* 敲击轴控制             */
#define ADXL345_REG_ACT_TAP_STATUS 0x2B    /* 活动/敲击状态          */
#define ADXL345_REG_BW_RATE        0x2C    /* 采样率 + 功耗模式      */
#define ADXL345_REG_POWER_CTL      0x2D    /* 电源控制               */
#define ADXL345_REG_INT_ENABLE     0x2E    /* 中断使能               */
#define ADXL345_REG_INT_MAP        0x2F    /* 中断映射               */
#define ADXL345_REG_INT_SOURCE     0x30    /* 中断源                 */
#define ADXL345_REG_DATA_FORMAT    0x31    /* 数据格式               */
#define ADXL345_REG_DATAX0         0x32    /* X 轴低字节             */
#define ADXL345_REG_DATAX1         0x33    /* X 轴高字节             */
#define ADXL345_REG_DATAY0         0x34    /* Y 轴低字节             */
#define ADXL345_REG_DATAY1         0x35    /* Y 轴高字节             */
#define ADXL345_REG_DATAZ0         0x36    /* Z 轴低字节             */
#define ADXL345_REG_DATAZ1         0x37    /* Z 轴高字节             */
#define ADXL345_REG_FIFO_CTL       0x38    /* FIFO 控制              */
#define ADXL345_REG_FIFO_STATUS    0x39    /* FIFO 状态              */

/* ========================== 量程 (Data Format 寄存器) ========================== */
typedef enum {
    ADXL345_RANGE_2G   = 0x00,
    ADXL345_RANGE_4G   = 0x01,
    ADXL345_RANGE_8G   = 0x02,
    ADXL345_RANGE_16G  = 0x03
} ADXL345_Range_t;

/* ========================== 采样率 (BW_RATE 寄存器) ========================== */
typedef enum {
    ADXL345_RATE_0_1HZ  = 0x00,
    ADXL345_RATE_0_2HZ  = 0x01,
    ADXL345_RATE_0_39HZ = 0x02,
    ADXL345_RATE_0_78HZ = 0x03,
    ADXL345_RATE_1_56HZ = 0x04,
    ADXL345_RATE_3_13HZ = 0x05,
    ADXL345_RATE_6_25HZ = 0x06,
    ADXL345_RATE_12_5HZ = 0x07,
    ADXL345_RATE_25HZ   = 0x08,
    ADXL345_RATE_50HZ   = 0x09,
    ADXL345_RATE_100HZ  = 0x0A,
    ADXL345_RATE_200HZ  = 0x0B,
    ADXL345_RATE_400HZ  = 0x0C,
    ADXL345_RATE_800HZ  = 0x0D,
    ADXL345_RATE_1600HZ = 0x0E,
    ADXL345_RATE_3200HZ = 0x0F
} ADXL345_Rate_t;

/* ========================== 中断类型 ========================== */
#define ADXL345_INT_DATA_READY    (1 << 7)
#define ADXL345_INT_SINGLE_TAP    (1 << 6)
#define ADXL345_INT_DOUBLE_TAP    (1 << 5)
#define ADXL345_INT_ACTIVITY      (1 << 4)
#define ADXL345_INT_INACTIVITY    (1 << 3)
#define ADXL345_INT_FREE_FALL     (1 << 2)
#define ADXL345_INT_WATERMARK     (1 << 1)
#define ADXL345_INT_OVERRUN       (1 << 0)

/* ========================== SPI 读写标志 ========================== */
#define ADXL345_SPI_READ          (1 << 7)
#define ADXL345_SPI_WRITE         (0 << 7)
#define ADXL345_SPI_MB            (1 << 6)   /* 多字节传输 */

/* ========================== API 函数声明 ========================== */

void     ADXL345_Init(void);
uint8_t  ADXL345_GetDevID(void);
void     ADXL345_SetRange(ADXL345_Range_t range);
void     ADXL345_SetRate(ADXL345_Rate_t rate);
void     ADXL345_EnableInterrupts(void);
void     ADXL345_ReadAccel(int16_t *x, int16_t *y, int16_t *z);
uint8_t  ADXL345_GetIntSource(void);

/* ---- 底层 SPI 操作 ---- */
void     ADXL345_WriteReg(uint8_t reg, uint8_t value);
uint8_t  ADXL345_ReadReg(uint8_t reg);
void     ADXL345_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len);

/* ---- 片选控制 ---- */
void     ADXL345_CS_Low(void);
void     ADXL345_CS_High(void);

#endif /* __HW_ADXL345_H */

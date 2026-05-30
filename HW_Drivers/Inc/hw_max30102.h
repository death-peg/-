/**
 * @file    hw_max30102.h
 * @brief   MAX30102 心率/血氧传感器驱动 - 头文件
 *
 * 通信接口: I2C1 (400kHz)
 * I2C 地址: 0x57 (7-bit)
 */

#ifndef __HW_MAX30102_H
#define __HW_MAX30102_H

#include "main.h"
#include <stdint.h>

/* ========================== MAX30102 寄存器地址 ========================== */
#define MAX30102_REG_INT_STATUS1        0x00
#define MAX30102_REG_INT_STATUS2        0x01
#define MAX30102_REG_INT_ENABLE1        0x02
#define MAX30102_REG_INT_ENABLE2        0x03
#define MAX30102_REG_FIFO_WR_PTR        0x04
#define MAX30102_REG_OVF_COUNTER        0x05
#define MAX30102_REG_FIFO_RD_PTR        0x06
#define MAX30102_REG_FIFO_DATA          0x07
#define MAX30102_REG_FIFO_CONFIG        0x08
#define MAX30102_REG_MODE_CONFIG        0x09
#define MAX30102_REG_SPO2_CONFIG        0x0A
#define MAX30102_REG_LED1_PA            0x0C    /* Red LED 电流 */
#define MAX30102_REG_LED2_PA            0x0D    /* IR LED 电流  */
#define MAX30102_REG_PILOT_PA           0x10
#define MAX30102_REG_MULTI_LED_CTRL1    0x11
#define MAX30102_REG_MULTI_LED_CTRL2    0x12
#define MAX30102_REG_TEMP_INTR          0x1F
#define MAX30102_REG_TEMP_FRAC          0x20
#define MAX30102_REG_TEMP_CONFIG        0x21
#define MAX30102_REG_PROX_INT_THRESH    0x30
#define MAX30102_REG_REV_ID             0xFE
#define MAX30102_REG_PART_ID            0xFF    /* 应读得 0x15 */

/* ========================== 中断使能位 ========================== */
#define MAX30102_INT_FIFO_AFULL         (1 << 4)
#define MAX30102_INT_DATA_RDY           (1 << 6)
#define MAX30102_INT_ALC_OVF            (1 << 5)
#define MAX30102_INT_PROX_INT           (1 << 7)
#define MAX30102_INT_DIE_TEMP_RDY       (1 << 1)

/* ========================== 工作模式 ========================== */
typedef enum {
    MAX30102_MODE_HEARTRATE  = 0x02,    /* 仅心率 (Red LED)           */
    MAX30102_MODE_SPO2       = 0x03,    /* 心率+血氧 (Red + IR)       */
    MAX30102_MODE_MULTI_LED  = 0x07     /* 多LED模式                  */
} MAX30102_Mode_t;

/* ========================== 采样率 ========================== */
typedef enum {
    MAX30102_SR_50   = 0x00,
    MAX30102_SR_100  = 0x01,
    MAX30102_SR_200  = 0x02,
    MAX30102_SR_400  = 0x03,
    MAX30102_SR_800  = 0x04,
    MAX30102_SR_1000 = 0x05,
    MAX30102_SR_1600 = 0x06,
    MAX30102_SR_3200 = 0x07
} MAX30102_SampleRate_t;

/* ========================== LED 电流 (0~50mA) ========================== */
typedef enum {
    MAX30102_LED_CURRENT_0MA   = 0x00,
    MAX30102_LED_CURRENT_4MA   = 0x01,
    MAX30102_LED_CURRENT_8MA   = 0x02,
    MAX30102_LED_CURRENT_12MA  = 0x03,
    MAX30102_LED_CURRENT_16MA  = 0x04,
    MAX30102_LED_CURRENT_20MA  = 0x05,
    MAX30102_LED_CURRENT_24MA  = 0x06,
    MAX30102_LED_CURRENT_28MA  = 0x07,
    MAX30102_LED_CURRENT_32MA  = 0x08,
    MAX30102_LED_CURRENT_36MA  = 0x09,
    MAX30102_LED_CURRENT_40MA  = 0x0A,
    MAX30102_LED_CURRENT_44MA  = 0x0B,
    MAX30102_LED_CURRENT_48MA  = 0x0C,
    MAX30102_LED_CURRENT_50MA  = 0x0D
} MAX30102_LEDCurrent_t;

/* ========================== 脉冲宽度 ========================== */
typedef enum {
    MAX30102_PW_69US   = 0x00,           /* 15-bit ADC               */
    MAX30102_PW_118US  = 0x01,           /* 16-bit ADC               */
    MAX30102_PW_215US  = 0x02,           /* 17-bit ADC               */
    MAX30102_PW_411US  = 0x03            /* 18-bit ADC               */
} MAX30102_PulseWidth_t;

/* ========================== API 函数声明 ========================== */

void     MAX30102_Init(void);
uint8_t  MAX30102_GetPartID(void);
void     MAX30102_Reset(void);
void     MAX30102_SetMode(MAX30102_Mode_t mode);
void     MAX30102_SetSampleRate(MAX30102_SampleRate_t rate);
void     MAX30102_SetPulseWidth(MAX30102_PulseWidth_t pw);
void     MAX30102_SetLEDCurrent(MAX30102_LEDCurrent_t red, MAX30102_LEDCurrent_t ir);
void     MAX30102_SetFIFOAlmostFull(uint8_t samples);
void     MAX30102_EnableInterrupt(uint8_t int_flags);
void     MAX30102_DisableInterrupt(uint8_t int_flags);
uint8_t  MAX30102_GetFIFOSampleCount(void);
void     MAX30102_ReadFIFO(uint32_t *ir, uint32_t *red);
void     MAX30102_ClearFIFO(void);
float    MAX30102_ReadTemperature(void);

/* ---- 底层 I2C 操作 ---- */
void     MAX30102_WriteReg(uint8_t reg, uint8_t value);
uint8_t  MAX30102_ReadReg(uint8_t reg);
void     MAX30102_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len);

#endif /* __HW_MAX30102_H */

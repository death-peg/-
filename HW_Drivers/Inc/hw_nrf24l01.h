/**
 * @file    hw_nrf24l01.h
 * @brief   nRF24L01+ 2.4GHz 无线收发驱动 - 头文件
 *
 * 通信接口: SPI2 (10MHz, Mode 0)
 * 用途:     向接收端发送心率/血压/撞击报警数据
 *
 * 地址: 5 字节, 频道: 2.400~2.525GHz (126ch, 1MHz step)
 * 速率: 250kbps / 1Mbps / 2Mbps
 * 输出: -18dBm ~ 0dBm
 */

#ifndef __HW_NRF24L01_H
#define __HW_NRF24L01_H

#include "main.h"
#include <stdint.h>

/* ========================== nRF24L01+ 寄存器地址 ========================== */
#define NRF24L01_REG_CONFIG         0x00
#define NRF24L01_REG_EN_AA          0x01
#define NRF24L01_REG_EN_RXADDR      0x02
#define NRF24L01_REG_SETUP_AW       0x03
#define NRF24L01_REG_SETUP_RETR     0x04
#define NRF24L01_REG_RF_CH          0x05
#define NRF24L01_REG_RF_SETUP       0x06
#define NRF24L01_REG_STATUS         0x07
#define NRF24L01_REG_OBSERVE_TX     0x08
#define NRF24L01_REG_RPD            0x09
#define NRF24L01_REG_RX_ADDR_P0     0x0A
#define NRF24L01_REG_RX_ADDR_P1     0x0B
#define NRF24L01_REG_TX_ADDR        0x10
#define NRF24L01_REG_RX_PW_P0       0x11
#define NRF24L01_REG_FIFO_STATUS    0x17
#define NRF24L01_REG_DYNPD          0x1C
#define NRF24L01_REG_FEATURE        0x1D

/* ========================== 命令 ========================== */
#define NRF24L01_CMD_R_REGISTER     0x00
#define NRF24L01_CMD_W_REGISTER     0x20
#define NRF24L01_CMD_R_RX_PAYLOAD   0x61
#define NRF24L01_CMD_W_TX_PAYLOAD   0xA0
#define NRF24L01_CMD_FLUSH_TX       0xE1
#define NRF24L01_CMD_FLUSH_RX       0xE2
#define NRF24L01_CMD_REUSE_TX_PL    0xE3
#define NRF24L01_CMD_NOP            0xFF

/* ========================== STATUS 寄存器位 ========================== */
#define NRF24L01_STATUS_RX_DR       (1 << 6)
#define NRF24L01_STATUS_TX_DS       (1 << 5)
#define NRF24L01_STATUS_MAX_RT      (1 << 4)
#define NRF24L01_STATUS_TX_FULL     (1 << 0)

/* ========================== 发射功率 ========================== */
typedef enum {
    NRF24L01_PWR_M18DBM = 0x00,   /* -18dBm */
    NRF24L01_PWR_M12DBM = 0x01,   /* -12dBm */
    NRF24L01_PWR_M6DBM  = 0x02,   /* -6dBm  */
    NRF24L01_PWR_0DBM   = 0x03    /* 0dBm   */
} NRF24L01_TXPower_t;

/* ========================== 数据速率 ========================== */
typedef enum {
    NRF24L01_RATE_1MBPS   = 0x00,
    NRF24L01_RATE_2MBPS   = 0x01,
    NRF24L01_RATE_250KBPS = 0x04
} NRF24L01_DataRate_t;

/* ========================== API 函数声明 ========================== */

void     NRF24L01_Init(void);
void     NRF24L01_SetChannel(uint8_t channel);
void     NRF24L01_SetTXPower(NRF24L01_TXPower_t power);
void     NRF24L01_SetDataRate(NRF24L01_DataRate_t rate);
void     NRF24L01_SetTXAddress(const uint8_t *addr);
void     NRF24L01_SetRXAddress(uint8_t pipe, const uint8_t *addr);
void     NRF24L01_SetTXMode(void);
void     NRF24L01_SetRXMode(void);
uint8_t  NRF24L01_SendPayload(uint8_t *data, uint8_t len);
uint8_t  NRF24L01_IsTXComplete(void);
uint8_t  NRF24L01_GetStatus(void);
void     NRF24L01_ClearInterrupts(void);
void     NRF24L01_FlushTX(void);

/* ---- 底层 SPI ---- */
uint8_t  NRF24L01_ReadReg(uint8_t reg);
void     NRF24L01_WriteReg(uint8_t reg, uint8_t value);
void     NRF24L01_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len);
void     NRF24L01_WriteMulti(uint8_t reg, const uint8_t *data, uint8_t len);

/* ---- 片选 & CE 控制 ---- */
void     NRF24L01_CSN_Low(void);
void     NRF24L01_CSN_High(void);
void     NRF24L01_CE_High(void);
void     NRF24L01_CE_Low(void);

#endif /* __HW_NRF24L01_H */

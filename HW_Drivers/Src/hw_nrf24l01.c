/**
 * @file    hw_nrf24l01.c
 * @brief   nRF24L01+ 2.4GHz 无线收发驱动实现
 *
 * 本设备工作模式: 纯 TX (手环 → 接收端单向报警)
 */

#include "hw_nrf24l01.h"

/* ========================== 片选 & CE ========================== */

void NRF24L01_CSN_Low(void)
{
    HAL_GPIO_WritePin(NRF24L01_CSN_PORT, NRF24L01_CSN_PIN, GPIO_PIN_RESET);
}

void NRF24L01_CSN_High(void)
{
    HAL_GPIO_WritePin(NRF24L01_CSN_PORT, NRF24L01_CSN_PIN, GPIO_PIN_SET);
}

void NRF24L01_CE_High(void)
{
    HAL_GPIO_WritePin(NRF24L01_CE_PORT, NRF24L01_CE_PIN, GPIO_PIN_SET);
}

void NRF24L01_CE_Low(void)
{
    HAL_GPIO_WritePin(NRF24L01_CE_PORT, NRF24L01_CE_PIN, GPIO_PIN_RESET);
}

/* ========================== 底层 SPI ========================== */

uint8_t NRF24L01_ReadReg(uint8_t reg)
{
    uint8_t tx_data, rx_data;

    NRF24L01_CSN_Low();
    tx_data = NRF24L01_CMD_R_REGISTER | (reg & 0x1F);
    HAL_SPI_Transmit(&NRF24L01_SPI, &tx_data, 1, 50);
    HAL_SPI_Receive(&NRF24L01_SPI, &rx_data, 1, 50);
    NRF24L01_CSN_High();

    return rx_data;
}

void NRF24L01_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t tx_data[2];

    NRF24L01_CSN_Low();
    tx_data[0] = NRF24L01_CMD_W_REGISTER | (reg & 0x1F);
    tx_data[1] = value;
    HAL_SPI_Transmit(&NRF24L01_SPI, tx_data, 2, 50);
    NRF24L01_CSN_High();
}

void NRF24L01_ReadMulti(uint8_t reg, uint8_t *data, uint8_t len)
{
    NRF24L01_CSN_Low();
    uint8_t tx_addr = NRF24L01_CMD_R_REGISTER | (reg & 0x1F);
    HAL_SPI_Transmit(&NRF24L01_SPI, &tx_addr, 1, 50);
    HAL_SPI_Receive(&NRF24L01_SPI, data, len, 50);
    NRF24L01_CSN_High();
}

void NRF24L01_WriteMulti(uint8_t reg, const uint8_t *data, uint8_t len)
{
    NRF24L01_CSN_Low();
    uint8_t tx_addr = NRF24L01_CMD_W_REGISTER | (reg & 0x1F);
    HAL_SPI_Transmit(&NRF24L01_SPI, &tx_addr, 1, 50);
    HAL_SPI_Transmit(&NRF24L01_SPI, (uint8_t *)data, len, 50);
    NRF24L01_CSN_High();
}

/* ========================== 初始化 ========================== */

void NRF24L01_Init(void)
{
    NRF24L01_CE_Low();
    NRF24L01_CSN_High();

    HAL_Delay(5);  /* 上电稳定 (nRF 需 > 1.5ms) */

    /* 自动应答: 禁用 (纯 TX 模式不需要) */
    NRF24L01_WriteReg(NRF24L01_REG_EN_AA, 0x00);

    /* RX 地址使能: 仅 Pipe0 */
    NRF24L01_WriteReg(NRF24L01_REG_EN_RXADDR, 0x01);

    /* 地址宽度: 5 字节 */
    NRF24L01_WriteReg(NRF24L01_REG_SETUP_AW, 0x03);

    /* 自动重传: 禁用 (手动重发由应用层控制) */
    NRF24L01_WriteReg(NRF24L01_REG_SETUP_RETR, 0x00);

    /* RF 频道: 默认 76 (2.476GHz) */
    NRF24L01_WriteReg(NRF24L01_REG_RF_CH, 76);

    /* RF 设置: 1Mbps, 0dBm */
    NRF24L01_WriteReg(NRF24L01_REG_RF_SETUP, 0x06);

    /* 动态负载长度: 启用 */
    NRF24L01_WriteReg(NRF24L01_REG_DYNPD, 0x01);
    NRF24L01_WriteReg(NRF24L01_REG_FEATURE, 0x04);

    /* CONFIG: PWR_UP=1, PRIM_RX=0 (TX), CRC=2byte */
    NRF24L01_WriteReg(NRF24L01_REG_CONFIG, 0x0E);

    /* 清除状态和 FIFO */
    NRF24L01_ClearInterrupts();
    NRF24L01_FlushTX();

    HAL_Delay(2);
}

/* ========================== 配置 ========================== */

void NRF24L01_SetChannel(uint8_t channel)
{
    if (channel > 125) channel = 125;
    NRF24L01_WriteReg(NRF24L01_REG_RF_CH, channel);
}

void NRF24L01_SetTXPower(NRF24L01_TXPower_t power)
{
    uint8_t reg = NRF24L01_ReadReg(NRF24L01_REG_RF_SETUP);
    reg &= ~0x06;
    reg |= ((uint8_t)power << 1) & 0x06;
    NRF24L01_WriteReg(NRF24L01_REG_RF_SETUP, reg);
}

void NRF24L01_SetDataRate(NRF24L01_DataRate_t rate)
{
    uint8_t reg = NRF24L01_ReadReg(NRF24L01_REG_RF_SETUP);
    reg &= ~0x28;
    reg |= (uint8_t)rate & 0x28;
    NRF24L01_WriteReg(NRF24L01_REG_RF_SETUP, reg);
}

void NRF24L01_SetTXAddress(const uint8_t *addr)
{
    NRF24L01_WriteMulti(NRF24L01_REG_TX_ADDR, addr, 5);
    /* TX 模式的 Pipe0 地址也必须匹配 (用于接收 ACK，即使我们禁用 ACK) */
    NRF24L01_WriteMulti(NRF24L01_REG_RX_ADDR_P0, addr, 5);
}

void NRF24L01_SetRXAddress(uint8_t pipe, const uint8_t *addr)
{
    if (pipe > 5) return;
    NRF24L01_WriteMulti(NRF24L01_REG_RX_ADDR_P0 + pipe, addr,
                        (pipe < 2) ? 5 : 1);  /* Pipe0/1 为全地址, Pipe2-5 仅1字节 */
}

/* ========================== 收发控制 ========================== */

void NRF24L01_SetTXMode(void)
{
    NRF24L01_CE_Low();

    uint8_t config = NRF24L01_ReadReg(NRF24L01_REG_CONFIG);
    config &= ~(1 << 0);  /* PRIM_RX = 0 → TX 模式 */
    NRF24L01_WriteReg(NRF24L01_REG_CONFIG, config);

    NRF24L01_CE_High();
    HAL_Delay(1);  /* CE 高电平保持 > 10us */
}

void NRF24L01_SetRXMode(void)
{
    NRF24L01_CE_Low();

    uint8_t config = NRF24L01_ReadReg(NRF24L01_REG_CONFIG);
    config |= (1 << 0);  /* PRIM_RX = 1 → RX 模式 */
    NRF24L01_WriteReg(NRF24L01_REG_CONFIG, config);

    NRF24L01_CE_High();
}

uint8_t NRF24L01_SendPayload(uint8_t *data, uint8_t len)
{
    if (len > 32) len = 32;  /* nRF24L01+ 最大 32 字节负载 */

    NRF24L01_CSN_Low();
    uint8_t cmd = NRF24L01_CMD_W_TX_PAYLOAD;
    HAL_SPI_Transmit(&NRF24L01_SPI, &cmd, 1, 50);
    HAL_SPI_Transmit(&NRF24L01_SPI, data, len, 50);
    NRF24L01_CSN_High();

    /* 等待发送完成 (最多等待 10ms) */
    uint32_t timeout = 10000;
    while (!(NRF24L01_GetStatus() & (NRF24L01_STATUS_TX_DS | NRF24L01_STATUS_MAX_RT)))
    {
        if (--timeout == 0) return 0;  /* 超时 */
    }

    NRF24L01_ClearInterrupts();
    return 1;  /* 成功 */
}

uint8_t NRF24L01_IsTXComplete(void)
{
    uint8_t status = NRF24L01_GetStatus();
    return (status & (NRF24L01_STATUS_TX_DS | NRF24L01_STATUS_MAX_RT)) ? 1 : 0;
}

uint8_t NRF24L01_GetStatus(void)
{
    /* 发送 NOP 命令，同时读回 STATUS */
    uint8_t tx_cmd = NRF24L01_CMD_NOP, rx_status;
    NRF24L01_CSN_Low();
    HAL_SPI_TransmitReceive(&NRF24L01_SPI, &tx_cmd, &rx_status, 1, 50);
    NRF24L01_CSN_High();
    return rx_status;
}

void NRF24L01_ClearInterrupts(void)
{
    /* 写 1 清除对应中断标志 */
    NRF24L01_WriteReg(NRF24L01_REG_STATUS,
                      NRF24L01_STATUS_RX_DR |
                      NRF24L01_STATUS_TX_DS |
                      NRF24L01_STATUS_MAX_RT);
}

void NRF24L01_FlushTX(void)
{
    NRF24L01_CSN_Low();
    uint8_t cmd = NRF24L01_CMD_FLUSH_TX;
    HAL_SPI_Transmit(&NRF24L01_SPI, &cmd, 1, 50);
    NRF24L01_CSN_High();
}

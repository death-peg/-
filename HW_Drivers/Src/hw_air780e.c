/**
 * @file    hw_air780e.c
 * @brief   合宙 Air780E 4G Cat.1 模块驱动实现
 *
 * AT 指令参考: 合宙 Air780E AT 指令手册 v2.0+
 * MQTT 指令: AT+MCONFIG / AT+MCONNECT / AT+MPUB
 */

#include "hw_air780e.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

/* ========================== 接收缓冲区 ========================== */
#define AIR780E_RX_BUF_SIZE     512
static uint8_t  rx_buf[AIR780E_RX_BUF_SIZE];
static uint16_t rx_len = 0;

/* ========================== 模块状态 ========================== */
static Air780E_State_t air780e_state = AIR780E_STATE_OFF;
static MQTT_Config_t   mqtt_config_storage;

/* ========================== 电源控制 ========================== */

/**
 * @brief  开机时序: PWRKEY 拉低 ≥1.2s
 */
void Air780E_PowerOn(void)
{
    HAL_GPIO_WritePin(AIR780E_PWRKEY_PORT, AIR780E_PWRKEY_PIN, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(1500));   /* 保持低电平 1.5s (安全余量) */
    HAL_GPIO_WritePin(AIR780E_PWRKEY_PORT, AIR780E_PWRKEY_PIN, GPIO_PIN_SET);

    air780e_state = AIR780E_STATE_BOOTING;
}

/**
 * @brief  关机: 长按 PWRKEY 或 AT+CPOWD=1
 */
void Air780E_PowerOff(void)
{
    Air780E_SendAT("AT+CPOWD=1\r\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    air780e_state = AIR780E_STATE_OFF;
}

/**
 * @brief  硬件复位: RESET 拉低 >50ms
 */
void Air780E_HardReset(void)
{
    HAL_GPIO_WritePin(AIR780E_RESET_PORT, AIR780E_RESET_PIN, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    HAL_GPIO_WritePin(AIR780E_RESET_PORT, AIR780E_RESET_PIN, GPIO_PIN_SET);

    air780e_state = AIR780E_STATE_BOOTING;
}

/* ========================== 底层 UART 操作 ========================== */

void Air780E_UART_Send(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&AIR780E_UART, (uint8_t *)data, len, 2000);
}

uint16_t Air780E_UART_Read(uint8_t *buf, uint16_t max_len, uint32_t timeout_ms)
{
    uint16_t total = 0;
    uint32_t start = xTaskGetTickCount();
    uint8_t  ch;

    while (total < max_len)
    {
        if (HAL_UART_Receive(&AIR780E_UART, &ch, 1, 10) == HAL_OK)
        {
            buf[total++] = ch;
        }
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms))
            break;
    }
    return total;
}

void Air780E_UART_Flush(void)
{
    uint8_t dummy;
    while (HAL_UART_Receive(&AIR780E_UART, &dummy, 1, 1) == HAL_OK);
}

/* ========================== AT 命令发送 ========================== */

void Air780E_SendAT(const char *cmd)
{
    Air780E_UART_Flush();
    Air780E_UART_Send((const uint8_t *)cmd, (uint16_t)strlen(cmd));
}

/**
 * @brief  等待响应中包含指定字符串
 * @return 1=成功, 0=超时
 */
uint8_t Air780E_WaitResponse(const char *expected, uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();

    memset(rx_buf, 0, AIR780E_RX_BUF_SIZE);
    rx_len = 0;

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms))
    {
        uint8_t ch;
        if (HAL_UART_Receive(&AIR780E_UART, &ch, 1, 5) == HAL_OK)
        {
            if (rx_len < AIR780E_RX_BUF_SIZE - 1)
                rx_buf[rx_len++] = ch;

            /* 简单检查: 收到 OK 或 ERROR 即返回 */
            if (strstr((const char *)rx_buf, expected) != NULL)
                return 1;
            if (strstr((const char *)rx_buf, "ERROR") != NULL)
                return 0;
        }
    }
    return 0;
}

/* ========================== 基础 AT 检查 ========================== */

uint8_t Air780E_ATTest(void)
{
    Air780E_SendAT("AT\r\n");
    return Air780E_WaitResponse("OK", 1000);
}

void Air780E_EchoOff(void)
{
    Air780E_SendAT("ATE0\r\n");
    Air780E_WaitResponse("OK", 500);
}

/* ========================== 等待模块就绪 ========================== */

uint8_t Air780E_WaitReady(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms))
    {
        /* 循环发送 AT 直到响应 */
        if (Air780E_ATTest())
        {
            air780e_state = AIR780E_STATE_INIT;
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    air780e_state = AIR780E_STATE_ERROR;
    return 0;
}

/* ========================== SIM 卡 / 网络 ========================== */

/**
 * @brief  检查 SIM 卡是否就绪
 *         返回: AT+CPIN? → +CPIN: READY
 */
uint8_t Air780E_SIMReady(void)
{
    Air780E_SendAT("AT+CPIN?\r\n");
    if (Air780E_WaitResponse("+CPIN: READY", 3000))
    {
        air780e_state = AIR780E_STATE_SIM_CHECK;
        return 1;
    }
    return 0;
}

/**
 * @brief  等待注册到网络 (4G Cat.1)
 * @return 1=注网成功, 0=超时
 */
uint8_t Air780E_NetRegister(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms))
    {
        Air780E_SendAT("AT+CGREG?\r\n");
        /* +CGREG: 0,1  或 +CGREG: 0,5 (已注册) */
        if (Air780E_WaitResponse("+CGREG: 0,1", 2000) ||
            Air780E_WaitResponse("+CGREG: 0,5", 2000))
        {
            air780e_state = AIR780E_STATE_NET_REG;
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    return 0;
}

/**
 * @brief  查询当前网络状态 (通过 NET_STATUS 引脚)
 */
NetStatus_t Air780E_GetNetStatus(void)
{
    GPIO_PinState pin = HAL_GPIO_ReadPin(AIR780E_NETSTAT_PORT, AIR780E_NETSTAT_PIN);

    /* Air780E NET_STATUS 引脚:
     *   常亮 → 已注网 (4G)
     *   200ms 周期闪烁 → 搜网中
     *   灭 → 关机/未注网
     */
    /* 简化: 高电平 = 已注网 */
    if (pin == GPIO_PIN_SET)
        return NET_STAT_4G;

    return NET_STAT_SEARCHING;
}

/**
 * @brief  读取信号强度 (CSQ)
 * @return 0~31 (越大越好), -1=失败
 */
int8_t Air780E_GetCSQ(void)
{
    Air780E_SendAT("AT+CSQ\r\n");
    if (Air780E_WaitResponse("+CSQ:", 2000))
    {
        /* 解析 +CSQ: <rssi>,<ber> */
        char *p = strstr((const char *)rx_buf, "+CSQ:");
        if (p)
        {
            int rssi;
            if (sscanf(p, "+CSQ: %d", &rssi) == 1)
                return (int8_t)rssi;
        }
    }
    return -1;
}

/* ========================== MQTT ========================== */

/**
 * @brief  配置 MQTT 参数 (必须在连接前调用)
 *         AT+MCONFIG=<client_id>,<username>,<password>,<keepalive>
 */
uint8_t Air780E_MQTTConfig(const MQTT_Config_t *cfg)
{
    if (!cfg) return 0;

    /* 保存配置 */
    memcpy(&mqtt_config_storage, cfg, sizeof(MQTT_Config_t));

    char at_cmd[256];
    snprintf(at_cmd, sizeof(at_cmd),
             "AT+MCONFIG=\"%s\",\"%s\",\"%s\",%u\r\n",
             cfg->client_id,
             cfg->username,
             cfg->password,
             cfg->keepalive);

    Air780E_SendAT(at_cmd);
    return Air780E_WaitResponse("OK", 3000);
}

/**
 * @brief  连接到 MQTT Broker
 *         AT+MCONNECT=1,\"<host>\",<port>
 */
uint8_t Air780E_MQTTConnect(uint32_t timeout_ms)
{
    char at_cmd[128];
    snprintf(at_cmd, sizeof(at_cmd),
             "AT+MCONNECT=1,\"%s\",%u\r\n",
             mqtt_config_storage.broker,
             mqtt_config_storage.port);

    Air780E_SendAT(at_cmd);

    /* 等待 CONNACK */
    uint32_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms))
    {
        if (Air780E_WaitResponse("CONNACK_OK", 1000))
        {
            air780e_state = AIR780E_STATE_MQTT_CONN;
            return 1;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return 0;
}

/**
 * @brief  发布 MQTT 消息
 *         AT+MPUB=\"<topic>\",<qos>,0,\"<payload>\"
 */
uint8_t Air780E_MQTTPublish(const char *topic, const char *payload)
{
    if (!topic || !payload) return 0;

    char at_cmd[512];
    snprintf(at_cmd, sizeof(at_cmd),
             "AT+MPUB=\"%s\",1,0,\"%s\"\r\n",
             topic, payload);

    Air780E_SendAT(at_cmd);
    return Air780E_WaitResponse("OK", 5000);
}

/**
 * @brief  发布原始二进制数据 (hex 编码)
 */
uint8_t Air780E_MQTTPublishRaw(const char *topic, const uint8_t *data, uint16_t len)
{
    if (!topic || !data || len == 0) return 0;

    /* 转为 hex 字符串 */
    char hex_buf[512];
    uint16_t hex_idx = 0;
    for (uint16_t i = 0; i < len && hex_idx < sizeof(hex_buf) - 3; i++)
    {
        hex_idx += snprintf(hex_buf + hex_idx, 3, "%02X", data[i]);
    }
    hex_buf[hex_idx] = '\0';

    return Air780E_MQTTPublish(topic, hex_buf);
}

/**
 * @brief  断开 MQTT
 */
uint8_t Air780E_MQTTDisconnect(void)
{
    Air780E_SendAT("AT+MDISCONNECT\r\n");
    return Air780E_WaitResponse("OK", 3000);
}

/**
 * @brief  检查 MQTT 是否保持连接
 */
uint8_t Air780E_MQTTIsConnected(void)
{
    Air780E_SendAT("AT+MQTTSTATUS?\r\n");
    return Air780E_WaitResponse("+MQTTSTATUS: 1", 2000);
}

/* ========================== 完整自动化初始化 ========================== */

/**
 * @brief  一键初始化 Air780E (开机 → SIM → 注网 → MQTT)
 * @param  cfg              MQTT 配置
 * @param  total_timeout_ms 总超时 (建议 30000ms+)
 * @return 1=成功进入 READY 状态, 0=超时失败
 */
uint8_t Air780E_AutoInit(const MQTT_Config_t *cfg, uint32_t total_timeout_ms)
{
    uint32_t start = xTaskGetTickCount();

    /* ---- Step 1: 开机 ---- */
    Air780E_PowerOn();

    /* 等待 UART 就绪 + AT 响应 (最多 15s) */
    if (!Air780E_WaitReady(15000))
    {
        /* 重试：硬件复位 */
        Air780E_HardReset();
        if (!Air780E_WaitReady(15000))
            return 0;
    }

    /* 关闭回显 */
    Air780E_EchoOff();
    vTaskDelay(pdMS_TO_TICKS(200));

    /* ---- Step 2: SIM 卡检测 ---- */
    if (!Air780E_SIMReady())
        return 0;

    /* ---- Step 3: 注网 (最多 20s) ---- */
    if (!Air780E_NetRegister(20000))
        return 0;

    /* ---- Step 4: MQTT 配置 + 连接 ---- */
    if (!Air780E_MQTTConfig(cfg))
        return 0;

    if (!Air780E_MQTTConnect(10000))
        return 0;

    air780e_state = AIR780E_STATE_READY;
    return 1;
}

Air780E_State_t Air780E_GetState(void)
{
    return air780e_state;
}

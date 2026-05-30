/**
 * @file    hw_air780e.h
 * @brief   合宙 Air780E 4G Cat.1 模块驱动 - 头文件
 *
 * 通信接口: UART3 (115200-8-N-1)
 * 协议:     AT 指令集 + MQTT
 * 电源:     3.1~4.5V 直接锂电池供电
 *
 * Air780E 开机时序:
 *   1. VBAT 上电 >3.1V
 *   2. PWRKEY 拉低 >1.2s → 模块开机
 *   3. 等待 UART 输出 "RDY" → 启动完成
 *   4. AT+CPIN? → SIM 卡就绪
 *   5. AT+CGATT? → 网络附着
 *   6. AT+MQTTCONN → 连接 MQTT Broker
 */

#ifndef __HW_AIR780E_H
#define __HW_AIR780E_H

#include "main.h"
#include <stdint.h>

/* ========================== 模块状态 ========================== */
typedef enum {
    AIR780E_STATE_OFF         = 0,     /* 关机                        */
    AIR780E_STATE_BOOTING     = 1,     /* 开机中 (等待 RDY)            */
    AIR780E_STATE_INIT        = 2,     /* AT 初始化                   */
    AIR780E_STATE_SIM_CHECK   = 3,     /* SIM 卡检测                 */
    AIR780E_STATE_NET_REG     = 4,     /* 搜网注网                    */
    AIR780E_STATE_MQTT_CONN   = 5,     /* MQTT 连接中                 */
    AIR780E_STATE_READY       = 6,     /* 就绪, 可收发                */
    AIR780E_STATE_ERROR       = 0xFF   /* 错误状态                    */
} Air780E_State_t;

/* ========================== MQTT 配置 ========================== */
typedef struct {
    char     broker[48];               /* MQTT 服务器地址             */
    uint16_t port;                     /* 端口 (默认 1883)           */
    char     client_id[32];            /* 客户端 ID (设备唯一标识)     */
    char     username[32];             /* 用户名 (可选)               */
    char     password[32];             /* 密码 (可选)                 */
    char     pub_topic[48];            /* 发布主题                    */
    char     heartbeat_topic[48];      /* 心跳主题                    */
    uint16_t keepalive;                /* 保活间隔 (秒, 建议 60)      */
} MQTT_Config_t;

/* ========================== 网络状态 ========================== */
typedef enum {
    NET_STAT_OFFLINE    = 0,
    NET_STAT_SEARCHING  = 1,
    NET_STAT_2G         = 2,
    NET_STAT_4G         = 4,
    NET_STAT_ERROR      = 0xFF
} NetStatus_t;

/* ========================== API ========================== */

/* ---- 电源 & 生命周期 ---- */
void         Air780E_PowerOn(void);
void         Air780E_PowerOff(void);
void         Air780E_HardReset(void);
uint8_t      Air780E_WaitReady(uint32_t timeout_ms);
Air780E_State_t Air780E_GetState(void);

/* ---- AT 基础通信 ---- */
void         Air780E_SendAT(const char *cmd);
uint8_t      Air780E_WaitResponse(const char *expected, uint32_t timeout_ms);
uint8_t      Air780E_ATTest(void);
void         Air780E_EchoOff(void);

/* ---- 网络 ---- */
uint8_t      Air780E_SIMReady(void);
uint8_t      Air780E_NetRegister(uint32_t timeout_ms);
NetStatus_t  Air780E_GetNetStatus(void);
int8_t       Air780E_GetCSQ(void);

/* ---- MQTT ---- */
uint8_t      Air780E_MQTTConfig(const MQTT_Config_t *cfg);
uint8_t      Air780E_MQTTConnect(uint32_t timeout_ms);
uint8_t      Air780E_MQTTPublish(const char *topic, const char *payload);
uint8_t      Air780E_MQTTPublishRaw(const char *topic, const uint8_t *data, uint16_t len);
uint8_t      Air780E_MQTTDisconnect(void);
uint8_t      Air780E_MQTTIsConnected(void);

/* ---- 完整自动化初始化 ---- */
uint8_t      Air780E_AutoInit(const MQTT_Config_t *cfg, uint32_t total_timeout_ms);

/* ---- 低层 UART ---- */
void         Air780E_UART_Send(const uint8_t *data, uint16_t len);
uint16_t     Air780E_UART_Read(uint8_t *buf, uint16_t max_len, uint32_t timeout_ms);
void         Air780E_UART_Flush(void);

#endif /* __HW_AIR780E_H */

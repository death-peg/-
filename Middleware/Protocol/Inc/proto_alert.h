/**
 * @file    proto_alert.h
 * @brief   报警通信协议 - 头文件
 *
 * 协议格式 (24 字节固定帧):
 *   [Sync 2B] [DevID 2B] [Type 1B] [Level 1B] [Value_H 1B] [Value_L 1B]
 *   [Timestamp 4B] [Reserved 4B] [Message 8B] [CRC 1B]
 *
 * 同步字: 0xAA 0x55
 * 设备 ID: 手环唯一标识 (出厂烧录)
 * Type: 报警类型 (心率/血压/撞击/低电量)
 * Level: 报警级别 (警告/危险/危急)
 */

#ifndef __PROTO_ALERT_H
#define __PROTO_ALERT_H

#include "app_manager.h"
#include <stdint.h>

/* ========================== 协议常量 ========================== */
#define PROTO_FRAME_MAX_LEN     32
#define PROTO_SYNC_WORD_H       0xAA
#define PROTO_SYNC_WORD_L       0x55
#define PROTO_DEVICE_ID         0x0001      /* 本设备编号 (可修改) */

/* ========================== 报警帧结构 ========================== */
#pragma pack(push, 1)
typedef struct {
    uint8_t  sync_h;           /* 同步字高字节 0xAA           */
    uint8_t  sync_l;           /* 同步字低字节 0x55           */
    uint16_t device_id;        /* 设备 ID (大端)              */
    uint8_t  alert_type;       /* 报警类型 AlertType_t         */
    uint8_t  alert_level;      /* 报警级别 AlertLevel_t        */
    uint8_t  value_h;          /* 报警值高字节                 */
    uint8_t  value_l;          /* 报警值低字节                 */
    uint32_t timestamp;        /* 时间戳 (ms, 大端)           */
    uint32_t reserved;         /* 保留字段                     */
    uint8_t  message[8];       /* 报警简述 (ASCII)             */
    uint8_t  crc8;             /* CRC-8 校验                   */
} Proto_AlertFrameRaw_t;
#pragma pack(pop)

/* 统一帧操作结构 */
typedef struct {
    uint8_t  raw_data[PROTO_FRAME_MAX_LEN];
    uint8_t  length;
} Proto_AlertFrame_t;

/* ========================== API 函数声明 ========================== */

void    Proto_BuildAlertFrame(Proto_AlertFrame_t *frame, const AlertEvent_t *event);
uint8_t Proto_ParseAlertFrame(const uint8_t *data, uint8_t len, AlertEvent_t *event);
uint8_t Proto_CalcCRC8(const uint8_t *data, uint8_t len);

#endif /* __PROTO_ALERT_H */

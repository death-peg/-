/**
 * @file    hw_vibrator.h
 * @brief   振动马达驱动 - 头文件
 *
 * 控制方式: GPIO (PA2) + PWM 可调强度
 * 用途:     报警触觉反馈
 */

#ifndef __HW_VIBRATOR_H
#define __HW_VIBRATOR_H

#include "main.h"
#include <stdint.h>

/* ========================== API 函数声明 ========================== */

void Vibrator_On(void);
void Vibrator_Off(void);
void Vibrator_Pulse(uint32_t duration_ms);       /* 持续振动 (阻塞)              */
void Vibrator_Beep(uint32_t duration_ms);         /* 短促振动 (非阻塞, 50%占空比) */
void Vibrator_Pattern(uint8_t pattern);           /* 预定义振动模式               */

/* ========================== 振动模式 ========================== */
#define VIBE_PATTERN_SHORT      0x01   /* 单短振 100ms           */
#define VIBE_PATTERN_DOUBLE     0x02   /* 双短振 100-100-100ms   */
#define VIBE_PATTERN_LONG       0x03   /* 长振 500ms             */
#define VIBE_PATTERN_SOS        0x04   /* SOS 模式               */

#endif /* __HW_VIBRATOR_H */

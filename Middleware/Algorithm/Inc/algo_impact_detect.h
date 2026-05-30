/**
 * @file    algo_impact_detect.h
 * @brief   撞击/跌倒检测算法 - 头文件
 *
 * 输入: ADXL345 三轴加速度 (200Hz)
 * 输出: 撞击标志
 *
 * 检测逻辑:
 *   1. 合加速度 > 阈值 → 疑似撞击
 *   2. 短时窗内加速度变化率 (Jerk) > 阈值 → 确认撞击
 *   3. 撞击后 1s 内姿态角变化 → 判断是否跌倒
 */

#ifndef __ALGO_IMPACT_DETECT_H
#define __ALGO_IMPACT_DETECT_H

#include "app_manager.h"
#include <stdint.h>

/* ========================== 检测参数 ========================== */
#define IMPACT_JERK_THRESHOLD   3.0f    /* 急动度阈值 (g/10ms)     */
#define IMPACT_WINDOW_SIZE      10      /* 检测窗口样本数 (50ms)   */
#define FALL_ANGLE_CHANGE       45.0f   /* 跌倒姿态角变化阈值(度)  */

/* ========================== API 函数声明 ========================== */

void    Algo_Impact_Init(void);
void    Algo_Impact_Process(Accel_Data_t *accel);
uint8_t Algo_Impact_IsImpactDetected(void);
uint8_t Algo_Impact_IsFallDetected(void);
void    Algo_Impact_Reset(void);

#endif /* __ALGO_IMPACT_DETECT_H */

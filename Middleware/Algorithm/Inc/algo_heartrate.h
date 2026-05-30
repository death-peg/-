/**
 * @file    algo_heartrate.h
 * @brief   心率计算算法 - 头文件
 *
 * 算法: 基于 PPG 信号的时域峰值检测
 *       输入: MAX30102 IR 通道原始值 (100Hz)
 *       输出: 心率 BPM
 */

#ifndef __ALGO_HEARTRATE_H
#define __ALGO_HEARTRATE_H

#include <stdint.h>

/* ========================== 算法参数 ========================== */
#define HR_SAMPLE_RATE          100         /* 输入数据采样率 (Hz)     */
#define HR_BUFFER_SIZE          256         /* 滑动窗口大小 (2.56s)    */
#define HR_PEAK_MIN_INTERVAL    30          /* 峰间最小间隔 (样本数)   */
                                             /* 30样本 @100Hz = 300ms  */
                                             /* 对应最大心率 200 BPM    */
#define HR_PEAK_THRESHOLD_FACTOR 0.6f       /* 动态阈值因子 (60%)      */

/* ========================== API 函数声明 ========================== */

void     Algo_HR_Init(void);
uint16_t Algo_HR_Process(uint32_t ir_raw);    /* 返回: 心率 (BPM), 0=未检出  */
uint8_t  Algo_HR_GetConfidence(void);         /* 返回: 置信度 0~100           */
void     Algo_HR_Reset(void);

#endif /* __ALGO_HEARTRATE_H */

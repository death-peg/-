/**
 * @file    algo_bp_estimation.h
 * @brief   血压估算算法 - 头文件
 *
 * 方法: 基于 PPG 波形特征参数 + PTT 估算
 *       收缩压 SBP, 舒张压 DBP
 *
 * 参考文献:
 *   - "Cuffless Blood Pressure Estimation Using Pulse Transit Time"
 *   - Moens-Korteweg 方程简化近似
 */

#ifndef __ALGO_BP_ESTIMATION_H
#define __ALGO_BP_ESTIMATION_H

#include "app_manager.h"
#include <stdint.h>

/* ========================== 算法参数 ========================== */
#define BP_CALIB_SYS_DEFAULT    120     /* 默认校准收缩压 (需用户校准) */
#define BP_CALIB_DIA_DEFAULT    80      /* 默认校准舒张压             */
#define BP_CALIB_COUNT          3       /* 校准所需读数次数           */

/* ========================== API 函数声明 ========================== */

void        Algo_BP_Init(void);
BP_Result_t Algo_BP_Estimate(PPG_RawData_t *ppg_data, uint8_t count);
void        Algo_BP_Calibrate(uint16_t ref_sys, uint16_t ref_dia);
uint8_t     Algo_BP_IsCalibrated(void);

#endif /* __ALGO_BP_ESTIMATION_H */

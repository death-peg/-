/**
 * @file    algo_bp_estimation.c
 * @brief   血压估算算法实现
 *
 * 简化模型 (基于 PPG 特征参数的线性回归):
 *   SBP = k1 * (IR_DC / IR_AC) + b1
 *   DBP = k2 * (IR_DC / IR_AC) + b2
 *
 * 其中:
 *   IR_DC: 红外光直流分量 (组织+静脉吸收)
 *   IR_AC: 红外光交流分量 (动脉搏动)
 *   IR_DC / IR_AC 近似于血管顺应性相关的参数
 *
 * 注意: 此方法仅为估算，精度受个体差异影响较大
 *       实际使用前需用标准血压计进行个体校准
 */

#include "algo_bp_estimation.h"
#include <string.h>
#include <math.h>

/* ========================== 内部状态 ========================== */

typedef struct {
    uint8_t  calibrated;
    float    calib_ir_dc;          /* 校准时 IR DC 平均值           */
    float    calib_ir_ac;          /* 校准时 IR AC 平均值           */
    uint16_t calib_sys;            /* 校准参考收缩压                */
    uint16_t calib_dia;            /* 校准参考舒张压                */

    float    sys_k;                /* SBP 比例系数                  */
    float    sys_b;                /* SBP 偏置                      */
    float    dia_k;                /* DBP 比例系数                  */
    float    dia_b;                /* DBP 偏置                      */
} BP_Algo_t;

static BP_Algo_t bp_algo;

/* ========================== 初始化 ========================== */

void Algo_BP_Init(void)
{
    memset(&bp_algo, 0, sizeof(bp_algo));

    /* 默认未校准，使用预设系数 */
    bp_algo.calibrated = 0;
    bp_algo.sys_k = 120.0f;
    bp_algo.sys_b = 60.0f;
    bp_algo.dia_k = 80.0f;
    bp_algo.dia_b = 40.0f;

    bp_algo.calib_sys = BP_CALIB_SYS_DEFAULT;
    bp_algo.calib_dia = BP_CALIB_DIA_DEFAULT;
}

/* ========================== 校准 ========================== */

void Algo_BP_Calibrate(uint16_t ref_sys, uint16_t ref_dia)
{
    bp_algo.calibrated = 1;
    bp_algo.calib_sys  = ref_sys;
    bp_algo.calib_dia  = ref_dia;

    /*
     * 实际校准流程：
     *   1. 用户佩戴手环，同时用标准血压计测量
     *   2. 记录 3 组参考值
     *   3. 同时读取传感器 IR_DC / IR_AC
     *   4. 计算个体化系数 k, b
     *
     * 此处简化: 使用默认系数
     */
}

uint8_t Algo_BP_IsCalibrated(void)
{
    return bp_algo.calibrated;
}

/* ========================== 血压估计 ========================== */

BP_Result_t Algo_BP_Estimate(PPG_RawData_t *ppg_data, uint8_t count)
{
    BP_Result_t result;
    memset(&result, 0, sizeof(result));

    if (count < 5) return result;

    /* ---- 计算 IR DC 和 AC 分量 ---- */
    float ir_sum  = 0.0f;
    float ir_min  = 1e9f;
    float ir_max  = -1e9f;

    for (uint8_t i = 0; i < count; i++)
    {
        float ir_val = (float)ppg_data[i].ir_value;
        ir_sum += ir_val;
        if (ir_val < ir_min) ir_min = ir_val;
        if (ir_val > ir_max) ir_max = ir_val;
    }

    float ir_dc = ir_sum / (float)count;   /* 平均值 ≈ DC */
    float ir_ac = ir_max - ir_min;          /* 峰峰值 ≈ AC */

    if (ir_ac < 1.0f || ir_dc < 100.0f) return result;

    /* ---- 特征参数 ---- */
    float perfusion_index = ir_ac / ir_dc;  /* 灌注指数 PI */

    /*
     * SBP 估算: SBP = a / PI + b
     * DBP 估算: DBP = a_d / PI + b_d
     *
     * 使用校准点计算系数:
     *   a_s = (SBP_cal - b_s) * PI_cal
     *   a_d = (DBP_cal - b_d) * PI_cal
     *
     * 简化常数 (来自文献经验值):
     */
    float a_s = 25.0f;    /* 收缩压经验系数 */
    float b_s = 65.0f;    /* 收缩压经验偏置 */
    float a_d = 18.0f;    /* 舒张压经验系数 */
    float b_d = 35.0f;    /* 舒张压经验偏置 */

    if (bp_algo.calibrated)
    {
        /* 使用校准值修正系数 (简化线性调整) */
        float scale = (float)bp_algo.calib_sys / BP_CALIB_SYS_DEFAULT;
        a_s *= scale;
        b_s *= scale;
        a_d *= scale;
        b_d *= scale;
    }

    float sbp_est = a_s / perfusion_index + b_s;
    float dbp_est = a_d / perfusion_index + b_d;

    /* ---- 合理性约束 ---- */
    if (sbp_est < 60.0f)  sbp_est = 60.0f;
    if (sbp_est > 250.0f) sbp_est = 250.0f;
    if (dbp_est < 30.0f)  dbp_est = 30.0f;
    if (dbp_est > 150.0f) dbp_est = 150.0f;

    result.systolic    = (uint16_t)(sbp_est + 0.5f);
    result.diastolic   = (uint16_t)(dbp_est + 0.5f);
    result.confidence  = bp_algo.calibrated ? 60 : 35;  /* 未校准则低置信度 */
    result.timestamp_ms = ppg_data[count - 1].timestamp_ms;

    return result;
}

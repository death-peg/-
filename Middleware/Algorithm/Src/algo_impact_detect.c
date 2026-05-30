/**
 * @file    algo_impact_detect.c
 * @brief   撞击/跌倒检测算法实现
 */

#include "algo_impact_detect.h"
#include <string.h>
#include <math.h>

/* ========================== 内部状态 ========================== */

typedef struct {
    /* 撞击检测 */
    float    mag_buffer[IMPACT_WINDOW_SIZE];
    uint8_t  buffer_index;
    uint8_t  impact_detected;
    uint32_t impact_timestamp;
    float    peak_magnitude;

    /* 跌倒检测 */
    uint8_t  fall_detected;
    float    pre_impact_angle;       /* 撞击前姿态角 (与水平夹角)  */
    float    post_impact_angle;      /* 撞击后姿态角               */
    uint8_t  checking_fall;

    /* 去抖动 */
    uint32_t last_impact_time;
    #define IMPACT_DEBOUNCE_MS    500
} Impact_Algo_t;

static Impact_Algo_t imp_algo;

/* ========================== 工具函数 ========================== */

/**
 * @brief  计算加速度矢量与水平面的夹角 (度)
 * @note   angle = acos(|az| / mag) * 180/PI
 */
static float CalcTiltAngle(float ax, float ay, float az)
{
    float mag = sqrtf(ax * ax + ay * ay + az * az);
    if (mag < 0.01f) return 90.0f;

    /* 竖直分量比 = |az| / mag, 夹角 = asin 或 acos */
    float ratio = fabsf(az) / mag;
    if (ratio > 1.0f) ratio = 1.0f;

    return acosf(ratio) * 57.29578f;   /* 180/PI */
}

/* ========================== 初始化 ========================== */

void Algo_Impact_Init(void)
{
    memset(&imp_algo, 0, sizeof(imp_algo));
}

void Algo_Impact_Reset(void)
{
    imp_algo.impact_detected = 0;
    imp_algo.fall_detected   = 0;
    imp_algo.checking_fall   = 0;
}

/* ========================== 数据处理 ========================== */

void Algo_Impact_Process(Accel_Data_t *accel)
{
    if (!accel) return;

    uint8_t  idx = imp_algo.buffer_index;
    uint32_t now = accel->timestamp_ms;

    /* ---- 更新环形缓冲区 ---- */
    imp_algo.mag_buffer[idx] = accel->magnitude;
    imp_algo.buffer_index = (idx + 1) % IMPACT_WINDOW_SIZE;

    /* ---- 计算急动度 (Jerk = Δmag / Δt) ---- */
    uint8_t prev_idx = (idx == 0) ? (IMPACT_WINDOW_SIZE - 1) : (idx - 1);
    float jerk = fabsf(accel->magnitude - imp_algo.mag_buffer[prev_idx]);

    /* ---- 撞击判定 ---- */
    if (accel->magnitude > IMPACT_WARNING_G && jerk > IMPACT_JERK_THRESHOLD)
    {
        /* 去抖动检查 */
        if ((now - imp_algo.last_impact_time) > IMPACT_DEBOUNCE_MS)
        {
            imp_algo.impact_detected = 1;
            imp_algo.impact_timestamp = now;
            imp_algo.peak_magnitude = accel->magnitude;
            imp_algo.last_impact_time = now;

            /* 记录撞击前姿态角 */
            imp_algo.pre_impact_angle = CalcTiltAngle(accel->ax, accel->ay, accel->az);
            imp_algo.checking_fall = 1;
        }
    }

    /* ---- 跌倒检测 (撞击后持续检查姿态) ---- */
    if (imp_algo.checking_fall)
    {
        /* 撞击后 500ms ~ 2000ms 检查姿态 */
        uint32_t elapsed = now - imp_algo.impact_timestamp;

        if (elapsed > 500 && elapsed < 2000)
        {
            float current_angle = CalcTiltAngle(accel->ax, accel->ay, accel->az);
            float angle_change = fabsf(current_angle - imp_algo.pre_impact_angle);

            if (angle_change > FALL_ANGLE_CHANGE && accel->magnitude < 1.5f)
            {
                /* 姿态角大幅变化 + 加速度恢复正常 → 疑似跌倒 */
                imp_algo.fall_detected = 1;
                imp_algo.post_impact_angle = current_angle;
            }
        }
        else if (elapsed > 2500)
        {
            /* 超时，停止跌倒检查 */
            imp_algo.checking_fall = 0;
        }
    }
}

/* ========================== 结果查询 ========================== */

uint8_t Algo_Impact_IsImpactDetected(void)
{
    uint8_t ret = imp_algo.impact_detected;
    imp_algo.impact_detected = 0;    /* 读取后自动清除 */
    return ret;
}

uint8_t Algo_Impact_IsFallDetected(void)
{
    uint8_t ret = imp_algo.fall_detected;
    imp_algo.fall_detected = 0;
    return ret;
}

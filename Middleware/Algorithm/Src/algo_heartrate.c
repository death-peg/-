/**
 * @file    algo_heartrate.c
 * @brief   心率计算算法实现
 *
 * 方法: 滑动窗口 + 动态阈值 + 峰值检测 + 中值滤波
 *
 * 处理流程:
 *   1. 原始 IR 值 → DC 偏置去除 (高通滤波)
 *   2. → 移动平均平滑
 *   3. → 动态阈值峰值检测
 *   4. → 峰间间隔统计 → 中值 BPM
 */

#include "algo_heartrate.h"
#include <string.h>

/* ========================== 内部状态 ========================== */

typedef struct {
    /* 信号缓冲区 */
    uint32_t raw_buffer[HR_BUFFER_SIZE];
    float    filtered_buffer[HR_BUFFER_SIZE];
    uint16_t buffer_index;

    /* 峰值检测状态 */
    uint16_t peak_count;
    uint16_t peak_indices[32];          /* 最近 32 个峰的位置      */
    uint32_t peak_intervals[32];        /* 峰间间隔 (样本数)       */
    uint8_t  peak_interval_count;

    /* 动态阈值 */
    float    max_value;
    float    min_value;
    float    threshold;

    /* DC 偏置追踪 */
    float    dc_estimate;
    float    dc_alpha;                  /* 低通滤波系数            */

    /* 结果 */
    uint16_t current_bpm;
    uint8_t  confidence;

    /* 预处理: 移动平均 */
    float    ma_buffer[8];
    uint8_t  ma_index;
} HR_Algo_t;

static HR_Algo_t hr_algo;

/* ========================== 初始化 ========================== */

void Algo_HR_Init(void)
{
    memset(&hr_algo, 0, sizeof(hr_algo));
    hr_algo.dc_alpha   = 0.99f;     /* DC 追踪慢速更新        */
    hr_algo.max_value   = 0.0f;
    hr_algo.min_value   = 1e9f;
    hr_algo.threshold   = 0.0f;
}

void Algo_HR_Reset(void)
{
    Algo_HR_Init();
}

/* ========================== 数据处理 ========================== */

uint16_t Algo_HR_Process(uint32_t ir_raw)
{
    float filtered;
    uint16_t idx = hr_algo.buffer_index;

    /* ---- Step 1: DC 偏置去除 ---- */
    hr_algo.dc_estimate = hr_algo.dc_alpha * hr_algo.dc_estimate
                        + (1.0f - hr_algo.dc_alpha) * (float)ir_raw;
    float ac_signal = (float)ir_raw - hr_algo.dc_estimate;

    /* ---- Step 2: 8 点移动平均平滑 ---- */
    hr_algo.ma_buffer[hr_algo.ma_index % 8] = ac_signal;
    hr_algo.ma_index++;

    float sum = 0.0f;
    uint8_t ma_count = (hr_algo.ma_index < 8) ? hr_algo.ma_index : 8;
    for (uint8_t i = 0; i < ma_count; i++)
        sum += hr_algo.ma_buffer[i];
    filtered = sum / (float)ma_count;

    /* ---- 存入缓冲区 ---- */
    hr_algo.raw_buffer[idx]      = ir_raw;
    hr_algo.filtered_buffer[idx] = filtered;
    hr_algo.buffer_index = (idx + 1) % HR_BUFFER_SIZE;

    /* ---- Step 3: 动态阈值峰值检测 ---- */
    uint16_t prev_idx = (idx == 0) ? (HR_BUFFER_SIZE - 1) : (idx - 1);
    float prev_val = hr_algo.filtered_buffer[prev_idx];
    float prev2_val = hr_algo.filtered_buffer[(prev_idx == 0) ? (HR_BUFFER_SIZE - 1) : (prev_idx - 1)];

    /* 更新 min/max */
    if (filtered > hr_algo.max_value) hr_algo.max_value = filtered;
    if (filtered < hr_algo.min_value) hr_algo.min_value = filtered;

    /* 动态阈值 = 最小值 + 振幅 * 60% */
    float amplitude = hr_algo.max_value - hr_algo.min_value;
    hr_algo.threshold = hr_algo.min_value + amplitude * HR_PEAK_THRESHOLD_FACTOR;

    /* 峰值条件: 当前 > 阈值, 前一点为局部最大 */
    if (filtered > hr_algo.threshold && prev_val > filtered && prev_val > prev2_val)
    {
        /* 检查与上一个峰的最小间隔 */
        if (hr_algo.peak_count > 0)
        {
            uint16_t last_peak = hr_algo.peak_indices[(hr_algo.peak_count - 1) % 32];
            uint32_t interval;

            if (prev_idx > last_peak)
                interval = prev_idx - last_peak;
            else
                interval = (HR_BUFFER_SIZE - last_peak) + prev_idx;

            if (interval >= HR_PEAK_MIN_INTERVAL)
            {
                /* 有效峰值 */
                hr_algo.peak_indices[hr_algo.peak_count % 32] = prev_idx;
                hr_algo.peak_intervals[hr_algo.peak_interval_count % 32] = interval;
                hr_algo.peak_count++;
                hr_algo.peak_interval_count++;
            }
        }
        else
        {
            /* 第一个峰值 */
            hr_algo.peak_indices[0] = prev_idx;
            hr_algo.peak_intervals[0] = 0;
            hr_algo.peak_count = 1;
        }

        /* 衰减 max，允许追踪下降趋势 */
        hr_algo.max_value *= 0.98f;
    }
    /* 谷值检测，衰减 min */
    if (filtered < hr_algo.min_value * 1.1f)
    {
        hr_algo.min_value *= 1.02f;
    }

    /* ---- Step 4: 计算 BPM ---- */
    if (hr_algo.peak_interval_count >= 3)
    {
        /* 取最近 8 个间隔的中值 */
        uint8_t num_intervals = (hr_algo.peak_interval_count < 8)
                                ? hr_algo.peak_interval_count : 8;
        uint32_t intervals[8];

        for (uint8_t i = 0; i < num_intervals; i++)
        {
            uint8_t src = (hr_algo.peak_interval_count - num_intervals + i) % 32;
            intervals[i] = hr_algo.peak_intervals[src];
        }

        /* 简单冒泡排序取中值 */
        for (uint8_t i = 0; i < num_intervals - 1; i++)
            for (uint8_t j = i + 1; j < num_intervals; j++)
                if (intervals[i] > intervals[j])
                {
                    uint32_t tmp = intervals[i];
                    intervals[i] = intervals[j];
                    intervals[j] = tmp;
                }

        uint32_t median_interval = intervals[num_intervals / 2];

        if (median_interval > 0)
        {
            /* BPM = 60 * SampleRate / Interval */
            float bpm = (60.0f * HR_SAMPLE_RATE) / (float)median_interval;

            /* BPM 合理性检查: 30~220 */
            if (bpm >= 30.0f && bpm <= 220.0f)
            {
                hr_algo.current_bpm = (uint16_t)(bpm + 0.5f);

                /* 置信度: 峰数量越多越可信 */
                if (num_intervals >= 6)
                    hr_algo.confidence = 80;
                else if (num_intervals >= 4)
                    hr_algo.confidence = 60;
                else
                    hr_algo.confidence = 40;

                return hr_algo.current_bpm;
            }
        }
    }

    hr_algo.confidence = 0;
    return 0;
}

uint8_t Algo_HR_GetConfidence(void)
{
    return hr_algo.confidence;
}

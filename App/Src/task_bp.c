/**
 * @file    task_bp.c
 * @brief   血压估算任务实现 (1Hz)
 *
 * 基于 PTT (Pulse Transit Time / 脉搏传导时间) 进行无袖带血压估算
 * 原理: PTT 与血压呈近似线性反比关系
 *       SBP ≈ a - b * ln(PTT)
 *       DBP ≈ c - d * ln(PTT)
 *
 * 注: 此方法精度有限，仅供健康参考，不可替代医用血压计
 */

#include "task_bp.h"
#include "algo_bp_estimation.h"
#include "proto_alert.h"
#include <string.h>
#include <stdio.h>

void Task_BloodPressure(void *argument)
{
    PPG_RawData_t  ppg_buf[20];      /* 1秒缓冲 (100Hz → 100点，取20) */
    BP_Result_t    bp_result;
    AlertEvent_t   alert;
    DisplayData_t  display;
    TickType_t     last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(1000);  /* 1Hz */

    uint8_t sample_idx = 0;

    (void)argument;

    /* 初始化血压估算算法 */
    Algo_BP_Init();
    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, period);

        /* ---- 收集 1 秒内的 PPG 数据 ---- */
        sample_idx = 0;
        TickType_t collect_start = xTaskGetTickCount();

        while ((xTaskGetTickCount() - collect_start) < pdMS_TO_TICKS(1000))
        {
            if (xQueueReceive(queue_ppg_raw, &ppg_buf[sample_idx],
                              pdMS_TO_TICKS(50)) == pdTRUE)
            {
                sample_idx++;
                if (sample_idx >= 20) break;
            }
        }

        /* ---- 运行血压估算算法 ---- */
        if (sample_idx >= 10)
        {
            bp_result = Algo_BP_Estimate(ppg_buf, sample_idx);

            if (bp_result.confidence > 30)   /* 置信度阈值 */
            {
                xQueueSend(queue_bp_result, &bp_result, 0);

                /* ---- 血压安全范围判断 ---- */
                if (bp_result.systolic > BP_SYS_DANGER ||
                    bp_result.diastolic > BP_DIA_DANGER)
                {
                    alert.type    = ALERT_TYPE_BLOOD_PRESSURE;
                    alert.level   = ALERT_LEVEL_DANGER;
                    alert.value   = bp_result.systolic;
                    alert.timestamp_ms = bp_result.timestamp_ms;
                    snprintf(alert.message, sizeof(alert.message),
                             "BP %d/%d DANGER!", bp_result.systolic, bp_result.diastolic);
                    xQueueSend(queue_alert_event, &alert, 0);
                    xEventGroupSetBits(event_alert_flags, EVENT_BIT_BP_ALERT);
                }
                else if (bp_result.systolic > BP_SYS_NORMAL_MAX ||
                         bp_result.diastolic > BP_DIA_NORMAL_MAX)
                {
                    alert.type    = ALERT_TYPE_BLOOD_PRESSURE;
                    alert.level   = ALERT_LEVEL_WARNING;
                    alert.value   = bp_result.systolic;
                    alert.timestamp_ms = bp_result.timestamp_ms;
                    snprintf(alert.message, sizeof(alert.message),
                             "BP %d/%d HIGH", bp_result.systolic, bp_result.diastolic);
                    xQueueSend(queue_alert_event, &alert, 0);
                }

                /* 更新显示 */
                display.systolic  = bp_result.systolic;
                display.diastolic = bp_result.diastolic;
                xQueueOverwrite(queue_display_data, &display);
            }
        }
    }
}

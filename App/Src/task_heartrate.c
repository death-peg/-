/**
 * @file    task_heartrate.c
 * @brief   心率监测任务实现 (100Hz)
 *
 * 流程: MAX30102 FIFO 读取 → PPG 滤波 → 心率计算 → 报警判断 → 发送显示
 */

#include "task_heartrate.h"
#include "hw_max30102.h"
#include "algo_heartrate.h"
#include "proto_alert.h"
#include <string.h>
#include <stdio.h>

/* ========================== 任务入口 ========================== */

void Task_HeartRate(void *argument)
{
    PPG_RawData_t  ppg_data;
    HR_Result_t    hr_result;
    AlertEvent_t   alert;
    DisplayData_t  display;
    TickType_t     last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(10);  /* 100Hz = 10ms 周期 */

    (void)argument;

    /* ---- 初始化 MAX30102 ---- */
    if (xSemaphoreTake(sem_i2c1_mutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        MAX30102_Init();
        MAX30102_SetMode(MAX30102_MODE_HEARTRATE);
        MAX30102_SetFIFOAlmostFull(17);              /* 采样后触发中断 */
        MAX30102_EnableInterrupt(MAX30102_INT_FIFO_AFULL);
        xSemaphoreGive(sem_i2c1_mutex);
    }

    /* 初始化算法 */
    Algo_HR_Init();
    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        /* ---- 精确周期延时 ---- */
        vTaskDelayUntil(&last_wake_time, period);

        /* ---- 从 MAX30102 FIFO 读取 PPG 数据 ---- */
        if (xSemaphoreTake(sem_i2c1_mutex, pdMS_TO_TICKS(5)) == pdTRUE)
        {
            uint8_t fifo_count = MAX30102_GetFIFOSampleCount();

            while (fifo_count > 0)
            {
                MAX30102_ReadFIFO(&ppg_data.ir_value, &ppg_data.red_value);
                ppg_data.timestamp_ms = xTaskGetTickCount();

                /* 送入 PPG 原始数据队列 (供血压算法等使用) */
                xQueueSend(queue_ppg_raw, &ppg_data, 0);

                /* ---- 运行心率算法 ---- */
                uint16_t bpm = Algo_HR_Process(ppg_data.ir_value);

                if (bpm > 0)
                {
                    hr_result.heart_rate  = bpm;
                    hr_result.confidence  = Algo_HR_GetConfidence();
                    hr_result.timestamp_ms = ppg_data.timestamp_ms;

                    /* 推送至报警模块 */
                    xQueueSend(queue_hr_result, &hr_result, 0);

                    /* ---- 心率安全范围判断 ---- */
                    if (bpm > HR_DANGER_HIGH || bpm < HR_DANGER_LOW)
                    {
                        alert.type    = ALERT_TYPE_HEART_RATE;
                        alert.level   = ALERT_LEVEL_DANGER;
                        alert.value   = bpm;
                        alert.timestamp_ms = hr_result.timestamp_ms;
                        snprintf(alert.message, sizeof(alert.message),
                                 "HR %d BPM - DANGER!", bpm);
                        xQueueSend(queue_alert_event, &alert, 0);
                        xEventGroupSetBits(event_alert_flags, EVENT_BIT_HR_ALERT);
                    }
                    else if (bpm > HR_WARNING_HIGH)
                    {
                        alert.type    = ALERT_TYPE_HEART_RATE;
                        alert.level   = ALERT_LEVEL_WARNING;
                        alert.value   = bpm;
                        alert.timestamp_ms = hr_result.timestamp_ms;
                        snprintf(alert.message, sizeof(alert.message),
                                 "HR %d BPM - HIGH", bpm);
                        xQueueSend(queue_alert_event, &alert, 0);
                    }

                    /* 更新显示数据 */
                    display.heart_rate = bpm;
                    xQueueOverwrite(queue_display_data, &display);
                }

                fifo_count--;
            }
            xSemaphoreGive(sem_i2c1_mutex);
        }
    }
}

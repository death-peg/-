/**
 * @file    task_impact.c
 * @brief   撞击检测任务实现 (200Hz)
 *
 * 流程: ADXL345 高速读取 → 合加速度计算 → 撞击判断 → 可疑跌倒检测 → 报警
 */

#include "task_impact.h"
#include "hw_adxl345.h"
#include "algo_impact_detect.h"
#include "proto_alert.h"
#include "hw_vibrator.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

void Task_ImpactDetect(void *argument)
{
    Accel_Data_t  accel;
    AlertEvent_t  alert;
    TickType_t    last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(5);  /* 200Hz = 5ms */

    (void)argument;

    /* ---- 初始化 ADXL345 ---- */
    if (xSemaphoreTake(sem_spi1_mutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        ADXL345_Init();
        ADXL345_SetRange(ADXL345_RANGE_16G);     /* ±16g 量程     */
        ADXL345_SetRate(ADXL345_RATE_200HZ);     /* 200Hz 采样率  */
        ADXL345_EnableInterrupts();              /* 使能撞击中断  */
        xSemaphoreGive(sem_spi1_mutex);
    }

    /* 初始化撞击检测算法 */
    Algo_Impact_Init();

    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, period);

        /* ---- 读取加速度数据 ---- */
        if (xSemaphoreTake(sem_spi1_mutex, pdMS_TO_TICKS(3)) == pdTRUE)
        {
            int16_t raw_x, raw_y, raw_z;
            ADXL345_ReadAccel(&raw_x, &raw_y, &raw_z);

            /* 转换为 g (16g 量程, 13-bit 分辨率) */
            accel.ax = raw_x * 0.004f;    /* 16g / 4096 = 3.9mg/LSB ≈ 0.004g/LSB */
            accel.ay = raw_y * 0.004f;
            accel.az = raw_z * 0.004f;

            /* 合加速度 */
            accel.magnitude = sqrtf(accel.ax * accel.ax +
                                    accel.ay * accel.ay +
                                    accel.az * accel.az);
            accel.timestamp_ms = xTaskGetTickCount();

            xSemaphoreGive(sem_spi1_mutex);

            /* ---- 撞击检测算法 ---- */
            Algo_Impact_Process(&accel);
            accel.is_impact = Algo_Impact_IsImpactDetected();

            /* ---- 安全阈判断 ---- */
            if (accel.magnitude > IMPACT_FALL_G)
            {
                /* 疑似跌倒 — 最高优先级报警 */
                alert.type    = ALERT_TYPE_IMPACT;
                alert.level   = ALERT_LEVEL_CRITICAL;
                alert.value   = (uint32_t)(accel.magnitude * 100);
                alert.timestamp_ms = accel.timestamp_ms;
                snprintf(alert.message, sizeof(alert.message),
                         "FALL! %.1fg", accel.magnitude);
                xQueueSend(queue_alert_event, &alert, 0);
                xEventGroupSetBits(event_alert_flags, EVENT_BIT_IMPACT_ALERT);

                /* 振动反馈 */
                Vibrator_Pulse(500);   /* 长振 500ms */
            }
            else if (accel.magnitude > IMPACT_DANGER_G)
            {
                alert.type    = ALERT_TYPE_IMPACT;
                alert.level   = ALERT_LEVEL_DANGER;
                alert.value   = (uint32_t)(accel.magnitude * 100);
                alert.timestamp_ms = accel.timestamp_ms;
                snprintf(alert.message, sizeof(alert.message),
                         "IMPACT %.1fg", accel.magnitude);
                xQueueSend(queue_alert_event, &alert, 0);
                xEventGroupSetBits(event_alert_flags, EVENT_BIT_IMPACT_ALERT);

                Vibrator_Pulse(300);   /* 中振 300ms */
            }
            else if (accel.magnitude > IMPACT_WARNING_G)
            {
                alert.type    = ALERT_TYPE_IMPACT;
                alert.level   = ALERT_LEVEL_WARNING;
                alert.value   = (uint32_t)(accel.magnitude * 100);
                alert.timestamp_ms = accel.timestamp_ms;
                snprintf(alert.message, sizeof(alert.message),
                         "LIGHT IMPACT %.1fg", accel.magnitude);
                xQueueSend(queue_alert_event, &alert, 0);
            }

            /* 送入加速度队列供其他模块 */
            xQueueSend(queue_accel_data, &accel, 0);
        }
    }
}

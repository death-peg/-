/**
 * @file    task_sysmon.c
 * @brief   系统监控任务实现 (1Hz)
 *
 * 监控内容:
 *   - 设备运行状态 (心跳输出到串口)
 *   - 传感器在线检测
 *   - 电池电压模拟 (可通过 ADC 实际读取)
 *   - 堆栈使用率 (FreeRTOS uxTaskGetStackHighWaterMark)
 */

#include "task_sysmon.h"
#include "hw_max30102.h"
#include "hw_adxl345.h"
#include <stdio.h>
#include <string.h>

/* ========================== 调试串口输出 ========================== */
static void DebugPrint(const char *msg)
{
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)msg, (uint16_t)strlen(msg), 50);
}

void Task_SystemMonitor(void *argument)
{
    DisplayData_t display;
    TickType_t    last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(1000);  /* 1Hz */

    uint32_t loop_count = 0;
    uint8_t  hr_sensor_ok = 0;
    uint8_t  accel_sensor_ok = 0;

    (void)argument;

    last_wake_time = xTaskGetTickCount();

    /* 上电检测传感器 */
    uint8_t max30102_id = MAX30102_GetPartID();
    uint8_t adxl345_id  = ADXL345_GetDevID();
    hr_sensor_ok    = (max30102_id == 0x15) ? 1 : 0;
    accel_sensor_ok = (adxl345_id  == 0xE5) ? 1 : 0;

    char buf[80];
    snprintf(buf, sizeof(buf),
             "\r\n[SmartBand] System Boot\r\n"
             "MAX30102 ID: 0x%02X %s\r\n"
             "ADXL345  ID: 0x%02X %s\r\n"
             "FreeRTOS Running...\r\n",
             max30102_id, hr_sensor_ok    ? "OK" : "FAIL",
             adxl345_id,  accel_sensor_ok ? "OK" : "FAIL");
    DebugPrint(buf);

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, period);
        loop_count++;

        /* ---- 每秒心跳输出 ---- */
        snprintf(buf, sizeof(buf),
                 "[%lu] HR_SENS:%d ACC_SENS:%d\r\n",
                 loop_count, hr_sensor_ok, accel_sensor_ok);
        DebugPrint(buf);

        /* ---- 模拟电池电量 (实际应读 ADC) ---- */
        /* 假设 3.7V 锂电池, 满电 4.2V, 放电截止 3.3V */
        /* ADC 读取公式: voltage = adc_value * 3.3V / 4096 (12-bit) */
        uint8_t batt_pct = 85;  /* 占位: 应用实际 ADC 读取 */

        /* ---- 任务栈使用监控 ---- */
        if (loop_count % 10 == 0)  /* 每 10 秒 */
        {
            UBaseType_t hr_highwater = uxTaskGetStackHighWaterMark(task_hr_handle);
            UBaseType_t impact_highwater = uxTaskGetStackHighWaterMark(task_impact_handle);
            snprintf(buf, sizeof(buf),
                     "[SysMon] Stack: HR=%lu Impact=%lu FreeHeap=%lu\r\n",
                     hr_highwater, impact_highwater,
                     xPortGetFreeHeapSize());
            DebugPrint(buf);
        }

        /* 更新显示 (电量) */
        display.battery_percent = batt_pct;
        xQueueOverwrite(queue_display_data, &display);
    }
}

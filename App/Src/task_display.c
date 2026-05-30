/**
 * @file    task_display.c
 * @brief   OLED 显示刷新任务实现 (10Hz)
 *
 * 职责：从显示数据队列获取最新数据，刷新 SSD1306 屏幕
 * 页面布局：
 *   Line 0: "SmartBand v1.0"
 *   Line 1-2: 心率 + 血压
 *   Line 3-4: 秒表计时
 *   Line 5-6: 撞击 G 值 / 报警状态
 *   Line 7: 电池电量
 */

#include "task_display.h"
#include "hw_oled_ssd1306.h"
#include <string.h>
#include <stdio.h>

/* ========================== 内部显示缓冲区 ========================== */
static DisplayData_t disp_cache = {0};
static uint8_t alert_blink = 0;

void Task_Display(void *argument)
{
    DisplayData_t  new_data;
    TickType_t     last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(100);  /* 10Hz */

    (void)argument;

    /* ---- 初始化 OLED ---- */
    if (xSemaphoreTake(sem_i2c2_mutex, pdMS_TO_TICKS(200)) == pdTRUE)
    {
        OLED_Init();
        OLED_Clear();
        OLED_ShowString(0, 0, "SmartBand v1.0", 12);
        OLED_Refresh();
        xSemaphoreGive(sem_i2c2_mutex);
    }

    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, period);

        alert_blink = !alert_blink;

        /* ---- 获取最新显示数据 (非阻塞) ---- */
        if (xQueueReceive(queue_display_data, &new_data, 0) == pdTRUE)
        {
            memcpy(&disp_cache, &new_data, sizeof(DisplayData_t));
        }

        /* ---- 获取互斥锁，刷新屏幕 ---- */
        if (xSemaphoreTake(sem_i2c2_mutex, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            char line[22];  /* SSD1306 128x64 → 每行约 21 个 6px 字符 */
            uint8_t y = 0;

            OLED_Clear();

            /* ---- 第1行: 心率 ---- */
            snprintf(line, sizeof(line), "HR: %3d BPM", disp_cache.heart_rate);
            OLED_ShowString(0, y, line, 12);
            y += 12;

            /* ---- 第2行: 血压 ---- */
            if (disp_cache.systolic > 0)
                snprintf(line, sizeof(line), "BP: %3d/%3d",
                         disp_cache.systolic, disp_cache.diastolic);
            else
                snprintf(line, sizeof(line), "BP: ---/---");
            OLED_ShowString(0, y, line, 12);
            y += 12;

            /* ---- 第3-4行: 秒表 ---- */
            uint32_t ms  = disp_cache.stopwatch_ms;
            uint8_t  min = (ms / 60000) % 100;
            uint8_t  sec = (ms / 1000) % 60;
            uint16_t cs  = (ms % 1000) / 10;        /* 百分秒 */
            snprintf(line, sizeof(line), "SW: %02d:%02d.%02d", min, sec, cs);
            OLED_ShowString(0, y, line, 12);
            y += 12;

            /* ---- 第5行: 加速度 / 撞击 ---- */
            snprintf(line, sizeof(line), "G: %.1f g",
                     disp_cache.accel_magnitude);
            OLED_ShowString(0, y, line, 12);
            y += 12;

            /* ---- 第6行: 报警状态 (闪烁) ---- */
            if (disp_cache.alert_active && alert_blink)
                OLED_ShowString(0, y, "!ALERT!", 12);
            else
                OLED_ShowString(0, y, "         ", 12);

            /* ---- 第7行: 电量 ---- */
            /* 超出 128x64 显示范围则不显示 */
            (void)disp_cache.battery_percent;

            OLED_Refresh();

            xSemaphoreGive(sem_i2c2_mutex);
        }
    }
}

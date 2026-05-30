/**
 * @file    task_stopwatch.c
 * @brief   秒表计时任务实现 (100Hz)
 *
 * 功能: 启动/停止/复位 秒表，支持分段计时 (Lap)
 * 控制: PA0 (Mode键) 短按 → 启动/停止, 长按 → 复位
 *       PA1 (Select键) 短按 → 记录分段
 */

#include "task_stopwatch.h"
#include <string.h>

/* ========================== 内部状态 ========================== */
static Stopwatch_Data_t sw_data = {0};
static uint32_t last_tim_count = 0;
static uint8_t  btn_mode_last_state = 1;
static uint8_t  btn_select_last_state = 1;
static uint32_t btn_mode_press_time = 0;
static uint8_t  btn_mode_long_detected = 0;

/**
 * @brief  读取按键去抖状态
 */
static uint8_t ReadButton_Mode(void)
{
    return HAL_GPIO_ReadPin(BTN_MODE_PORT, BTN_MODE_PIN);
}

static uint8_t ReadButton_Select(void)
{
    return HAL_GPIO_ReadPin(BTN_SELECT_PORT, BTN_SELECT_PIN);
}

/* ========================== 任务入口 ========================== */

void Task_Stopwatch(void *argument)
{
    DisplayData_t  display;
    TickType_t     last_wake_time;
    const TickType_t period = pdMS_TO_TICKS(10);  /* 100Hz */

    (void)argument;

    /* 初始化显示数据 */
    memset(&display, 0, sizeof(display));

    last_wake_time = xTaskGetTickCount();

    while (1)
    {
        vTaskDelayUntil(&last_wake_time, period);

        /* ======== 按键扫描 ======== */
        uint8_t btn_mode_now   = ReadButton_Mode();
        uint8_t btn_select_now = ReadButton_Select();

        /* ---- MODE 键: 短按启动/停止, 长按(>1s)复位 ---- */
        if (btn_mode_last_state == 1 && btn_mode_now == 0)
        {
            /* 按下 */
            btn_mode_press_time = xTaskGetTickCount();
            btn_mode_long_detected = 0;
        }
        else if (btn_mode_last_state == 0 && btn_mode_now == 1)
        {
            /* 释放 */
            if (!btn_mode_long_detected)
            {
                /* 短按: 启动/停止 */
                if (sw_data.running)
                {
                    sw_data.running = 0;            /* 暂停 */
                }
                else
                {
                    sw_data.running = 1;            /* 开始 */
                    last_tim_count = __HAL_TIM_GET_COUNTER(&STOPWATCH_TIM);
                }
            }
        }
        else if (btn_mode_now == 0 && !btn_mode_long_detected)
        {
            /* 持续按住检测长按 */
            if ((xTaskGetTickCount() - btn_mode_press_time) > pdMS_TO_TICKS(1000))
            {
                btn_mode_long_detected = 1;
                /* 长按: 复位秒表 */
                sw_data.running    = 0;
                sw_data.elapsed_ms = 0;
                sw_data.lap_count  = 0;
                memset(sw_data.lap_times, 0, sizeof(sw_data.lap_times));
            }
        }
        btn_mode_last_state = btn_mode_now;

        /* ---- SELECT 键: 记录 Lap ---- */
        if (btn_select_last_state == 1 && btn_select_now == 0)
        {
            if (sw_data.running && sw_data.lap_count < 10)
            {
                sw_data.lap_times[sw_data.lap_count] = sw_data.elapsed_ms;
                sw_data.lap_count++;
            }
        }
        btn_select_last_state = btn_select_now;

        /* ======== 计时更新 ======== */
        if (sw_data.running)
        {
            uint32_t now = __HAL_TIM_GET_COUNTER(&STOPWATCH_TIM);
            uint32_t delta;

            /* 处理 16 位计数器溢出 */
            if (now >= last_tim_count)
                delta = now - last_tim_count;
            else
                delta = (0xFFFF - last_tim_count) + now + 1;

            last_tim_count = now;

            /* delta 单位 0.1ms (10kHz 计数), 转为 ms */
            sw_data.elapsed_ms += delta / 10;
        }

        /* ======== 更新显示 ======== */
        display.stopwatch_ms = sw_data.elapsed_ms;
        xQueueOverwrite(queue_display_data, &display);
    }
}

/**
 * @file    hw_vibrator.c
 * @brief   振动马达驱动实现
 */

#include "hw_vibrator.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

/* ========================== 基本控制 ========================== */

void Vibrator_On(void)
{
    HAL_GPIO_WritePin(VIBRATOR_PORT, VIBRATOR_PIN, GPIO_PIN_SET);
}

void Vibrator_Off(void)
{
    HAL_GPIO_WritePin(VIBRATOR_PORT, VIBRATOR_PIN, GPIO_PIN_RESET);
}

/* ========================== 定时振动 ========================== */

void Vibrator_Pulse(uint32_t duration_ms)
{
    Vibrator_On();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    Vibrator_Off();
}

void Vibrator_Beep(uint32_t duration_ms)
{
    /* 短促振动 50% 占空比，非精确 */
    Vibrator_On();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    Vibrator_Off();
}

/* ========================== 振动模式 ========================== */

void Vibrator_Pattern(uint8_t pattern)
{
    switch (pattern)
    {
        case VIBE_PATTERN_SHORT:
            Vibrator_Pulse(100);
            break;

        case VIBE_PATTERN_DOUBLE:
            Vibrator_Pulse(100);
            vTaskDelay(pdMS_TO_TICKS(100));
            Vibrator_Pulse(100);
            break;

        case VIBE_PATTERN_LONG:
            Vibrator_Pulse(500);
            break;

        case VIBE_PATTERN_SOS:
            /* ... --- ... 摩尔斯 SOS */
            for (int i = 0; i < 3; i++) { Vibrator_Pulse(200); vTaskDelay(pdMS_TO_TICKS(200)); }
            vTaskDelay(pdMS_TO_TICKS(400));
            for (int i = 0; i < 3; i++) { Vibrator_Pulse(600); vTaskDelay(pdMS_TO_TICKS(200)); }
            vTaskDelay(pdMS_TO_TICKS(400));
            for (int i = 0; i < 3; i++) { Vibrator_Pulse(200); vTaskDelay(pdMS_TO_TICKS(200)); }
            break;

        default:
            break;
    }
}

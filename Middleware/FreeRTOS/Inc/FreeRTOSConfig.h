/**
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS 内核配置文件 (STM32L431RCT6)
 *
 * 适配 CMSIS-RTOS2 API，支持抢占式调度
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32l4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 调度策略 ========================== */
#define configUSE_PREEMPTION                    1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
#define configUSE_TICKLESS_IDLE                 1       /* 低功耗 Tickless 模式 */
#define configCPU_CLOCK_HZ                      (SystemCoreClock)
#define configTICK_RATE_HZ                      1000U   /* 1ms 滴答周期         */
#define configMAX_PRIORITIES                    7       /* 0~6 共7个优先级      */
#define configMINIMAL_STACK_SIZE                128     /* 最小任务栈 (字)       */
#define configMAX_TASK_NAME_LEN                 16
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3

/* ========================== 内存管理 ========================== */
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    ((size_t)24 * 1024)  /* 24KB 堆 */
#define configAPPLICATION_ALLOCATED_HEAP         0

/* ========================== 同步机制 ========================== */
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               6
#define configTIMER_QUEUE_LENGTH                8
#define configTIMER_TASK_STACK_DEPTH            256

/* ========================== 可选功能 ========================== */
#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ========================== 运行时统计 ========================== */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ========================== 中断嵌套 ========================== */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY          (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ========================== Hook 函数 ========================== */
#define configASSERT(x)   if((x) == 0) { portDISABLE_INTERRUPTS(); for(;;); }

/* 前向声明 TaskHandle_t (FreeRTOS 内核尚未包含 task.h) */
struct tskTaskControlBlock;
typedef struct tskTaskControlBlock *TaskHandle_t;

extern void vApplicationMallocFailedHook(void);
extern void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

/* ========================== 可选 API ========================== */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_xTaskGetIdleTaskHandle          0
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_CONFIG_H */

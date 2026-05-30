/**
 * @file    cmsis_os2.c
 * @brief   CMSIS-RTOS2 API → FreeRTOS 适配层实现
 *
 * 该文件实现 CMSIS-RTOS2 v2.1.3 API 的核心函数，
 * 底层使用 FreeRTOS 原生 API。
 *
 * 由于本项目主要使用 FreeRTOS 原生 API (xTaskCreate 等)，
 * 此文件仅提供必要的桥接函数 (osKernelStart 等)。
 * 完整 CMSIS-RTOS2 适配可在此文件基础上扩展。
 *
 * 参考: CMSIS-RTOS2 API v2.1.3 + FreeRTOS Kernel v10.x
 */

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "timers.h"

/* ========================== 辅助宏: 优先级转换 ========================== */

/**
 * @brief  CMSIS-RTOS2 osPriority → FreeRTOS 优先级映射
 *
 * CMSIS-RTOS2 定义:
 *   osPriorityNone          =  0
 *   osPriorityIdle          =  1
 *   osPriorityLow           =  8
 *   osPriorityLow1..7       =  8..15
 *   osPriorityBelowNormal   = 16
 *   osPriorityNormal        = 24
 *   osPriorityAboveNormal   = 32
 *   osPriorityHigh          = 40
 *   osPriorityRealtime      = 48
 *   osPriorityRealtime1..7  = 49..55
 *   osPriorityISR           = 56
 *
 * FreeRTOS 优先级: 0 (最低) ~ configMAX_PRIORITIES-1 (最高)
 */
static UBaseType_t prvMapPriority(osPriority_t priority)
{
    /* 本项目使用 configMAX_PRIORITIES = 7 (0~6) */
    if (priority >= osPriorityRealtime)   return (UBaseType_t)(configMAX_PRIORITIES - 1);
    if (priority >= osPriorityHigh)       return (UBaseType_t)(configMAX_PRIORITIES - 2);
    if (priority >= osPriorityAboveNormal) return (UBaseType_t)(configMAX_PRIORITIES - 3);
    if (priority >= osPriorityNormal)     return (UBaseType_t)(configMAX_PRIORITIES - 4);
    if (priority >= osPriorityBelowNormal) return (UBaseType_t)2;
    if (priority >= osPriorityLow)        return (UBaseType_t)1;
    return (UBaseType_t)0;
}

/* ========================== 内核初始化与启动 ========================== */

/**
 * @brief  初始化 RTOS 内核。
 *         FreeRTOS 无需显式初始化，此函数留空。
 */
osStatus_t osKernelInitialize(void)
{
    /* FreeRTOS 内核在第一个任务创建时自动初始化 */
    return osOK;
}

/**
 * @brief  获取内核状态。
 */
osKernelState_t osKernelGetState(void)
{
    switch (xTaskGetSchedulerState())
    {
        case taskSCHEDULER_RUNNING:
            return osKernelRunning;
        case taskSCHEDULER_SUSPENDED:
            return osKernelLocked;
        default:
            return osKernelReady;
    }
}

/**
 * @brief  启动 RTOS 内核调度器 (永不返回)。
 */
osStatus_t osKernelStart(void)
{
    /* 启动 FreeRTOS 调度器 — 永不返回 */
    vTaskStartScheduler();

    /* 不应到达此处，但以防万一 */
    for (;;) { __nop(); }
}

/**
 * @brief  获取系统 Tick 计数。
 */
uint32_t osKernelGetTickCount(void)
{
    TickType_t ticks;

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        return 0U;
    }

    ticks = xTaskGetTickCount();
    return (uint32_t)ticks;
}

/**
 * @brief  获取 Tick 频率 (Hz)。
 */
uint32_t osKernelGetTickFreq(void)
{
    return (uint32_t)configTICK_RATE_HZ;
}

/* ========================== 线程管理 ========================== */

/**
 * @brief  创建新线程。
 */
osThreadId_t osThreadNew(osThreadFunc_t func, void *argument,
                          const osThreadAttr_t *attr)
{
    TaskHandle_t   xHandle      = NULL;
    const char    *name         = "CMSIS";
    uint32_t       stackSize    = configMINIMAL_STACK_SIZE;
    UBaseType_t    priority     = (UBaseType_t)(configMAX_PRIORITIES - 1);
    BaseType_t     ret;

    if (attr != NULL)
    {
        if (attr->name       != NULL)  name      = attr->name;
        if (attr->stack_size != 0U)    stackSize = attr->stack_size;
        priority = prvMapPriority(attr->priority);
    }

    ret = xTaskCreate((TaskFunction_t)func,
                      name,
                      (configSTACK_DEPTH_TYPE)stackSize,
                      argument,
                      priority,
                      &xHandle);

    if (ret != pdPASS)
    {
        return NULL;
    }

    return (osThreadId_t)xHandle;
}

/**
 * @brief  获取当前线程 ID。
 */
osThreadId_t osThreadGetId(void)
{
    return (osThreadId_t)xTaskGetCurrentTaskHandle();
}

/**
 * @brief  获取线程名称。
 */
const char *osThreadGetName(osThreadId_t thread_id)
{
    return pcTaskGetName((TaskHandle_t)thread_id);
}

/**
 * @brief  终止当前线程。
 */
__NO_RETURN void osThreadExit(void)
{
    vTaskDelete(NULL);
    for (;;) { __nop(); }
}

/**
 * @brief  设置线程优先级。
 */
osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
{
    UBaseType_t uxPriority = prvMapPriority(priority);

    if (thread_id == NULL)
    {
        thread_id = (osThreadId_t)xTaskGetCurrentTaskHandle();
    }

    vTaskPrioritySet((TaskHandle_t)thread_id, uxPriority);
    return osOK;
}

/**
 * @brief  获取线程优先级。
 */
osPriority_t osThreadGetPriority(osThreadId_t thread_id)
{
    UBaseType_t uxPriority;

    if (thread_id == NULL)
    {
        thread_id = (osThreadId_t)xTaskGetCurrentTaskHandle();
    }

    uxPriority = uxTaskPriorityGet((TaskHandle_t)thread_id);

    /* 反向映射粗略估计 */
    if (uxPriority >= 5) return osPriorityRealtime;
    if (uxPriority >= 4) return osPriorityHigh;
    if (uxPriority >= 3) return osPriorityAboveNormal;
    if (uxPriority >= 2) return osPriorityNormal;
    if (uxPriority >= 1) return osPriorityLow;
    return osPriorityIdle;
}

/**
 * @brief  让出 CPU 给同优先级线程。
 */
osStatus_t osThreadYield(void)
{
    taskYIELD();
    return osOK;
}

/**
 * @brief  挂起线程。
 */
osStatus_t osThreadSuspend(osThreadId_t thread_id)
{
    if (thread_id == NULL)
    {
        thread_id = (osThreadId_t)xTaskGetCurrentTaskHandle();
    }

    vTaskSuspend((TaskHandle_t)thread_id);
    return osOK;
}

/**
 * @brief  恢复线程。
 */
osStatus_t osThreadResume(osThreadId_t thread_id)
{
    if (thread_id == NULL)
    {
        return osErrorParameter;
    }

    vTaskResume((TaskHandle_t)thread_id);
    return osOK;
}

/**
 * @brief  获取线程栈空间高水位。
 */
uint32_t osThreadGetStackSpace(osThreadId_t thread_id)
{
    if (thread_id == NULL)
    {
        thread_id = (osThreadId_t)xTaskGetCurrentTaskHandle();
    }

    return (uint32_t)uxTaskGetStackHighWaterMark((TaskHandle_t)thread_id);
}

/* ========================== 延时函数 ========================== */

/**
 * @brief  相对延时 (毫秒)。
 */
osStatus_t osDelay(uint32_t ticks)
{
    vTaskDelay(pdMS_TO_TICKS(ticks));
    return osOK;
}

/**
 * @brief  绝对延时 (系统 Tick)。
 */
osStatus_t osDelayUntil(uint32_t ticks)
{
    TickType_t xTicks = (TickType_t)ticks;
    vTaskDelayUntil(&xTicks, 0);
    return osOK;
}

/* ========================== 定时器管理 ========================== */

/**
 * @brief  创建一次性/周期性软件定时器。
 */
osTimerId_t osTimerNew(osTimerFunc_t func, osTimerType_t type,
                        void *argument, const osTimerAttr_t *attr)
{
    TimerHandle_t  xTimer;
    const char    *name = "CMSIS_Timer";
    UBaseType_t    reload = (type == osTimerPeriodic) ? pdTRUE : pdFALSE;
    uint32_t       period = 100U;

    if (attr != NULL)
    {
        if (attr->name != NULL) name = attr->name;
    }

    xTimer = xTimerCreate(name,
                          pdMS_TO_TICKS(period),
                          reload,
                          argument,
                          (TimerCallbackFunction_t)func);

    return (osTimerId_t)xTimer;
}

/**
 * @brief  启动定时器。
 */
osStatus_t osTimerStart(osTimerId_t timer_id, uint32_t ticks)
{
    TickType_t xTicks = pdMS_TO_TICKS(ticks);

    if (xTimerChangePeriod((TimerHandle_t)timer_id, xTicks, 0) != pdPASS)
    {
        return osError;
    }

    if (xTimerStart((TimerHandle_t)timer_id, 0) != pdPASS)
    {
        return osError;
    }

    return osOK;
}

/**
 * @brief  停止定时器。
 */
osStatus_t osTimerStop(osTimerId_t timer_id)
{
    if (xTimerStop((TimerHandle_t)timer_id, 0) != pdPASS)
    {
        return osError;
    }
    return osOK;
}

/**
 * @brief  删除定时器。
 */
osStatus_t osTimerDelete(osTimerId_t timer_id)
{
    if (xTimerDelete((TimerHandle_t)timer_id, 0) != pdPASS)
    {
        return osError;
    }
    return osOK;
}

/* ========================== 事件标志 ========================== */

/**
 * @brief  创建事件标志组。
 */
osEventFlagsId_t osEventFlagsNew(const osEventFlagsAttr_t *attr)
{
    EventGroupHandle_t xEventGroup;
    (void)attr;

    xEventGroup = xEventGroupCreate();
    if (xEventGroup == NULL)
    {
        return NULL;
    }

    /* 确保事件组创建干净 (无残留标志) */
    xEventGroupClearBits(xEventGroup, 0x00FFFFFFUL);
    return (osEventFlagsId_t)xEventGroup;
}

/**
 * @brief  设置事件标志。
 */
uint32_t osEventFlagsSet(osEventFlagsId_t ef_id, uint32_t flags)
{
    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)ef_id;
    EventBits_t        uxBits;

    if (xEventGroup == NULL)
    {
        return (uint32_t)osFlagsErrorParameter;
    }

    uxBits = xEventGroupSetBits(xEventGroup, (EventBits_t)flags);
    return (uint32_t)uxBits;
}

/**
 * @brief  等待事件标志。
 */
uint32_t osEventFlagsWait(osEventFlagsId_t ef_id, uint32_t flags,
                           uint32_t options, uint32_t timeout)
{
    EventGroupHandle_t xEventGroup = (EventGroupHandle_t)ef_id;
    EventBits_t        uxBits;
    TickType_t         xTicksToWait = (timeout == osWaitForever)
                                      ? portMAX_DELAY
                                      : pdMS_TO_TICKS(timeout);

    if (xEventGroup == NULL)
    {
        return (uint32_t)osFlagsErrorParameter;
    }

    /* 确定等待模式 */
    if ((options & osFlagsWaitAll) != 0U)
    {
        /* 等待所有标志 */
        uxBits = xEventGroupWaitBits(xEventGroup,
                                     (EventBits_t)flags,
                                     pdTRUE,   /* 等待后清除 */
                                     pdTRUE,   /* 等待所有位  */
                                     xTicksToWait);
    }
    else
    {
        uxBits = xEventGroupWaitBits(xEventGroup,
                                     (EventBits_t)flags,
                                     pdTRUE,   /* 等待后清除 */
                                     pdFALSE,  /* 等待任意位  */
                                     xTicksToWait);
    }

    if ((uxBits & (EventBits_t)flags) == (EventBits_t)flags)
    {
        return (uint32_t)uxBits;
    }

    /* 超时或错误 */
    return (uint32_t)osFlagsErrorTimeout;
}

/* ========================== 互斥量 ========================== */

/**
 * @brief  创建互斥量。
 */
osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
    SemaphoreHandle_t xMutex;
    (void)attr;

    xMutex = xSemaphoreCreateMutex();
    return (osMutexId_t)xMutex;
}

/**
 * @brief  获取互斥量。
 */
osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout)
{
    TickType_t xTicksToWait = (timeout == osWaitForever)
                              ? portMAX_DELAY
                              : pdMS_TO_TICKS(timeout);

    if (xSemaphoreTake((SemaphoreHandle_t)mutex_id, xTicksToWait) != pdTRUE)
    {
        return osErrorTimeout;
    }
    return osOK;
}

/**
 * @brief  释放互斥量。
 */
osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
    if (xSemaphoreGive((SemaphoreHandle_t)mutex_id) != pdTRUE)
    {
        return osErrorResource;
    }
    return osOK;
}

/**
 * @brief  删除互斥量。
 */
osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
    vSemaphoreDelete((SemaphoreHandle_t)mutex_id);
    return osOK;
}

/* ========================== 信号量 ========================== */

/**
 * @brief  创建信号量。
 */
osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count,
                                const osSemaphoreAttr_t *attr)
{
    SemaphoreHandle_t xSemaphore;
    (void)attr;

    xSemaphore = xSemaphoreCreateCounting((UBaseType_t)max_count,
                                          (UBaseType_t)initial_count);
    return (osSemaphoreId_t)xSemaphore;
}

/**
 * @brief  获取信号量。
 */
osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphore_id, uint32_t timeout)
{
    TickType_t xTicksToWait = (timeout == osWaitForever)
                              ? portMAX_DELAY
                              : pdMS_TO_TICKS(timeout);

    if (xSemaphoreTake((SemaphoreHandle_t)semaphore_id, xTicksToWait) != pdTRUE)
    {
        return osErrorTimeout;
    }
    return osOK;
}

/**
 * @brief  释放信号量。
 */
osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphore_id)
{
    if (xSemaphoreGive((SemaphoreHandle_t)semaphore_id) != pdTRUE)
    {
        return osErrorResource;
    }
    return osOK;
}

/**
 * @brief  删除信号量。
 */
osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphore_id)
{
    vSemaphoreDelete((SemaphoreHandle_t)semaphore_id);
    return osOK;
}

/* ========================== 消息队列 ========================== */

/**
 * @brief  创建消息队列。
 */
osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size,
                                      const osMessageQueueAttr_t *attr)
{
    QueueHandle_t xQueue;
    (void)attr;

    xQueue = xQueueCreate((UBaseType_t)msg_count, (UBaseType_t)msg_size);
    return (osMessageQueueId_t)xQueue;
}

/**
 * @brief  向队列发送消息。
 */
osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr,
                              uint8_t msg_prio, uint32_t timeout)
{
    TickType_t xTicksToWait = (timeout == osWaitForever)
                              ? portMAX_DELAY
                              : pdMS_TO_TICKS(timeout);
    (void)msg_prio;

    if (xQueueSendToBack((QueueHandle_t)mq_id, msg_ptr, xTicksToWait) != pdTRUE)
    {
        return osErrorTimeout;
    }
    return osOK;
}

/**
 * @brief  从队列接收消息。
 */
osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr,
                              uint8_t *msg_prio, uint32_t timeout)
{
    TickType_t xTicksToWait = (timeout == osWaitForever)
                              ? portMAX_DELAY
                              : pdMS_TO_TICKS(timeout);

    if (msg_prio != NULL)
    {
        *msg_prio = 0U;
    }

    if (xQueueReceive((QueueHandle_t)mq_id, msg_ptr, xTicksToWait) != pdTRUE)
    {
        return osErrorTimeout;
    }
    return osOK;
}

/**
 * @brief  获取队列容量。
 */
uint32_t osMessageQueueGetCount(osMessageQueueId_t mq_id)
{
    return (uint32_t)uxQueueMessagesWaiting((QueueHandle_t)mq_id);
}

/**
 * @brief  删除消息队列。
 */
osStatus_t osMessageQueueDelete(osMessageQueueId_t mq_id)
{
    vQueueDelete((QueueHandle_t)mq_id);
    return osOK;
}

/**
 * @file    app_manager.c
 * @brief   智能手环 - 应用管理器实现
 *
 * 负责 FreeRTOS 内核对象初始化 & 所有任务的创建
 */

#include "app_manager.h"

/* ---- Task 函数声明 ---- */
extern void Task_HeartRate(void *argument);
extern void Task_BloodPressure(void *argument);
extern void Task_ImpactDetect(void *argument);
extern void Task_AlertSend(void *argument);
extern void Task_Stopwatch(void *argument);
extern void Task_Display(void *argument);
extern void Task_SystemMonitor(void *argument);

/* ========================== 全局 IPC 对象定义 ========================== */
QueueHandle_t        queue_ppg_raw       = NULL;
QueueHandle_t        queue_hr_result     = NULL;
QueueHandle_t        queue_bp_result     = NULL;
QueueHandle_t        queue_accel_data    = NULL;
QueueHandle_t        queue_alert_event   = NULL;
QueueHandle_t        queue_display_data  = NULL;

SemaphoreHandle_t    sem_i2c1_mutex      = NULL;
SemaphoreHandle_t    sem_i2c2_mutex      = NULL;
SemaphoreHandle_t    sem_spi1_mutex      = NULL;

EventGroupHandle_t   event_alert_flags   = NULL;

/* ========================== 任务句柄 ========================== */
TaskHandle_t task_hr_handle       = NULL;
TaskHandle_t task_bp_handle       = NULL;
TaskHandle_t task_impact_handle   = NULL;
TaskHandle_t task_alert_handle    = NULL;
TaskHandle_t task_stopwatch_handle = NULL;
TaskHandle_t task_display_handle  = NULL;
TaskHandle_t task_sysmon_handle   = NULL;

/* ========================== 任务栈大小 & 优先级定义 ========================== */
#define STACK_HR           512
#define STACK_BP           384
#define STACK_IMPACT       512
#define STACK_ALERT        512
#define STACK_STOPWATCH    256
#define STACK_DISPLAY      384
#define STACK_SYSMON       256

#define PRIO_IMPACT        (osPriority_t)5    /* 最高: 撞击检测   */
#define PRIO_HR            (osPriority_t)4    /* 心率采样         */
#define PRIO_ALERT         (osPriority_t)3    /* 报警发送         */
#define PRIO_STOPWATCH     (osPriority_t)2    /* 秒表计时         */
#define PRIO_BP            (osPriority_t)2    /* 血压估算         */
#define PRIO_DISPLAY       (osPriority_t)1    /* 屏幕刷新         */
#define PRIO_SYSMON        (osPriority_t)0    /* 最低: 系统监控   */

/* ========================== Hook 回调 ========================== */

void vApplicationMallocFailedHook(void)
{
    /* 内存分配失败 - 可在此处记录或复位 */
    taskDISABLE_INTERRUPTS();
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* 栈溢出 - 可在此处记录或复位 */
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    while (1);
}

/* ========================== IPC 对象创建 ========================== */

void App_Manager_CreateIPC(void)
{
    /* 消息队列 - 长度根据缓冲区需求设定 */
    queue_ppg_raw       = xQueueCreate(32, sizeof(PPG_RawData_t));
    queue_hr_result     = xQueueCreate(8,  sizeof(HR_Result_t));
    queue_bp_result     = xQueueCreate(8,  sizeof(BP_Result_t));
    queue_accel_data    = xQueueCreate(16, sizeof(Accel_Data_t));
    queue_alert_event   = xQueueCreate(16, sizeof(AlertEvent_t));
    queue_display_data  = xQueueCreate(4,  sizeof(DisplayData_t));

    /* 互斥信号量 - 外设总线保护 */
    sem_i2c1_mutex = xSemaphoreCreateMutex();
    sem_i2c2_mutex = xSemaphoreCreateMutex();
    sem_spi1_mutex = xSemaphoreCreateMutex();

    /* 报警事件组 */
    event_alert_flags = xEventGroupCreate();

    /* 校验 */
    configASSERT(queue_ppg_raw != NULL);
    configASSERT(queue_hr_result != NULL);
    configASSERT(queue_bp_result != NULL);
    configASSERT(queue_accel_data != NULL);
    configASSERT(queue_alert_event != NULL);
    configASSERT(queue_display_data != NULL);
    configASSERT(sem_i2c1_mutex != NULL);
    configASSERT(sem_i2c2_mutex != NULL);
    configASSERT(sem_spi1_mutex != NULL);
    configASSERT(event_alert_flags != NULL);
}

/* ========================== 任务创建 ========================== */

void App_Manager_CreateTasks(void)
{
    BaseType_t ret;

    /* Task_ImpactDetect — 撞击检测 (最高优先级, 200Hz) */
    ret = xTaskCreate(Task_ImpactDetect, "Impact",
                      STACK_IMPACT, NULL, PRIO_IMPACT, &task_impact_handle);
    configASSERT(ret == pdPASS);

    /* Task_HeartRate — 心率采样+计算 (100Hz) */
    ret = xTaskCreate(Task_HeartRate, "HeartRate",
                      STACK_HR, NULL, PRIO_HR, &task_hr_handle);
    configASSERT(ret == pdPASS);

    /* Task_AlertSend — 报警无线发送 (事件驱动) */
    ret = xTaskCreate(Task_AlertSend, "Alert",
                      STACK_ALERT, NULL, PRIO_ALERT, &task_alert_handle);
    configASSERT(ret == pdPASS);

    /* Task_Stopwatch — 秒表计时 (100Hz) */
    ret = xTaskCreate(Task_Stopwatch, "StopWatch",
                      STACK_STOPWATCH, NULL, PRIO_STOPWATCH, &task_stopwatch_handle);
    configASSERT(ret == pdPASS);

    /* Task_BloodPressure — 血压估算 (1Hz) */
    ret = xTaskCreate(Task_BloodPressure, "BP_Est",
                      STACK_BP, NULL, PRIO_BP, &task_bp_handle);
    configASSERT(ret == pdPASS);

    /* Task_Display — OLED 显示刷新 (10Hz) */
    ret = xTaskCreate(Task_Display, "Display",
                      STACK_DISPLAY, NULL, PRIO_DISPLAY, &task_display_handle);
    configASSERT(ret == pdPASS);

    /* Task_SystemMonitor — 系统状态监控 (1Hz) */
    ret = xTaskCreate(Task_SystemMonitor, "SysMon",
                      STACK_SYSMON, NULL, PRIO_SYSMON, &task_sysmon_handle);
    configASSERT(ret == pdPASS);
}

/* ========================== 初始化入口 ========================== */

void App_Manager_Init(void)
{
    App_Manager_CreateIPC();
    App_Manager_CreateTasks();
}

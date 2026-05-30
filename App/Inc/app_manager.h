/**
 * @file    app_manager.h
 * @brief   智能手环 - 应用管理器头文件
 *
 * 职责：
 *   1. 创建所有 FreeRTOS 任务
 *   2. 定义全局 IPC 对象（队列/信号量/互斥锁/事件组）
 *   3. 系统状态管理
 */

#ifndef __APP_MANAGER_H
#define __APP_MANAGER_H

#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

/* ========================== 安全阈值配置 ========================== */

/* 心率阈值 (BPM) */
#define HR_NORMAL_MIN          60U
#define HR_NORMAL_MAX          100U
#define HR_WARNING_HIGH        120U
#define HR_DANGER_HIGH         150U
#define HR_DANGER_LOW          40U

/* 血压阈值 (mmHg) */
#define BP_SYS_NORMAL_MAX      140U     /* 收缩压正常高值    */
#define BP_SYS_DANGER          180U     /* 收缩压危险值      */
#define BP_DIA_NORMAL_MAX      90U      /* 舒张压正常高值    */
#define BP_DIA_DANGER          120U     /* 舒张压危险值      */

/* 撞击阈值 (g, 1g = 9.8m/s²) */
#define IMPACT_WARNING_G       2.0f     /* 轻度撞击          */
#define IMPACT_DANGER_G        5.0f     /* 严重撞击          */
#define IMPACT_FALL_G          8.0f     /* 疑似跌倒          */

/* ========================== 枚举定义 ========================== */

/* 报警级别 */
typedef enum {
    ALERT_LEVEL_NONE    = 0,
    ALERT_LEVEL_WARNING = 1,
    ALERT_LEVEL_DANGER  = 2,
    ALERT_LEVEL_CRITICAL = 3
} AlertLevel_t;

/* 报警类型 */
typedef enum {
    ALERT_TYPE_HEART_RATE    = 0x01,
    ALERT_TYPE_BLOOD_PRESSURE = 0x02,
    ALERT_TYPE_IMPACT        = 0x04,
    ALERT_TYPE_LOW_BATTERY   = 0x08,
    ALERT_TYPE_DEVICE_OFFLINE = 0x10
} AlertType_t;

/* 传感器数据类型 */
typedef enum {
    SENSOR_HEARTRATE = 0,
    SENSOR_BLOODPRESSURE,
    SENSOR_ACCELEROMETER,
    SENSOR_COUNT
} SensorType_t;

/* ========================== 数据结构 ========================== */

/* PPG 原始数据 */
typedef struct {
    uint32_t ir_value;          /* 红外通道值       */
    uint32_t red_value;         /* 红光通道值       */
    uint32_t timestamp_ms;      /* 时间戳(ms)       */
} PPG_RawData_t;

/* 心率计算结果 */
typedef struct {
    uint16_t heart_rate;        /* 心率 (BPM)       */
    uint8_t  confidence;        /* 置信度 0~100     */
    uint32_t timestamp_ms;
} HR_Result_t;

/* 血压数据 */
typedef struct {
    uint16_t systolic;          /* 收缩压 (mmHg)    */
    uint16_t diastolic;         /* 舒张压 (mmHg)    */
    uint8_t  confidence;        /* 置信度 0~100     */
    uint32_t timestamp_ms;
} BP_Result_t;

/* 加速度/撞击数据 */
typedef struct {
    float ax, ay, az;           /* 三轴加速度 (g)   */
    float magnitude;            /* 合加速度 (g)     */
    uint8_t is_impact;          /* 是否检测到撞击   */
    uint32_t timestamp_ms;
} Accel_Data_t;

/* 秒表状态 */
typedef struct {
    uint32_t elapsed_ms;        /* 已计时 (ms)      */
    uint8_t  running;           /* 是否运行中       */
    uint32_t lap_times[10];     /* 分段计时 (最多10段) */
    uint8_t  lap_count;
} Stopwatch_Data_t;

/* 报警事件包 */
typedef struct {
    AlertType_t  type;          /* 报警类型         */
    AlertLevel_t level;         /* 报警级别         */
    uint32_t     value;         /* 报警相关数值      */
    char         message[32];   /* 报警描述          */
    uint32_t     timestamp_ms;
} AlertEvent_t;

/* 显示数据 (发送至显示任务) */
typedef struct {
    uint16_t heart_rate;
    uint16_t systolic;
    uint16_t diastolic;
    float    accel_magnitude;
    uint32_t stopwatch_ms;
    uint8_t  alert_active;
    uint8_t  battery_percent;
} DisplayData_t;

/* ========================== 全局 IPC 对象 ========================== */

/* 消息队列 */
extern QueueHandle_t queue_ppg_raw;          /* PPG 原始数据 → 心率算法      */
extern QueueHandle_t queue_hr_result;        /* 心率结果 → 报警/显示         */
extern QueueHandle_t queue_bp_result;        /* 血压结果 → 报警/显示         */
extern QueueHandle_t queue_accel_data;       /* 加速度数据 → 撞击检测        */
extern QueueHandle_t queue_alert_event;      /* 报警事件 → 报警发送/显示     */
extern QueueHandle_t queue_display_data;     /* 显示数据 → 显示任务          */

/* 信号量 (二值/互斥) */
extern SemaphoreHandle_t sem_i2c1_mutex;     /* I2C1 总线互斥 (MAX30102)     */
extern SemaphoreHandle_t sem_i2c2_mutex;     /* I2C2 总线互斥 (OLED)         */
extern SemaphoreHandle_t sem_spi1_mutex;     /* SPI1 总线互斥 (ADXL345)      */

/* MQTT 默认配置 */
#define MQTT_BROKER_ADDR        "broker.emqx.io"        /* 公共测试 Broker, 生产换阿里云 */
#define MQTT_BROKER_PORT        1883
#define MQTT_CLIENT_ID          "SmartBand_0001"        /* 设备唯一编号, 量产需逐个写入   */
#define MQTT_USERNAME           ""
#define MQTT_PASSWORD           ""
#define MQTT_PUB_TOPIC          "smartband/alert"       /* 报警发布主题                  */
#define MQTT_HEARTBEAT_TOPIC    "smartband/heartbeat"   /* 心跳主题                      */
#define MQTT_KEEPALIVE_SEC      60

/* 事件组 */
extern EventGroupHandle_t event_alert_flags; /* 报警事件标志组               */

/* 事件位定义 */
#define EVENT_BIT_HR_ALERT         (1 << 0)
#define EVENT_BIT_BP_ALERT         (1 << 1)
#define EVENT_BIT_IMPACT_ALERT     (1 << 2)
#define EVENT_BIT_LOW_BATTERY      (1 << 3)
#define EVENT_BIT_DEVICE_OFFLINE   (1 << 4)
#define EVENT_BIT_ALL_ALERTS       (0x1F)

/* ========================== 任务句柄 ========================== */
extern TaskHandle_t task_hr_handle;
extern TaskHandle_t task_bp_handle;
extern TaskHandle_t task_impact_handle;
extern TaskHandle_t task_alert_handle;
extern TaskHandle_t task_stopwatch_handle;
extern TaskHandle_t task_display_handle;
extern TaskHandle_t task_sysmon_handle;

/* ========================== 函数声明 ========================== */
void App_Manager_Init(void);

void App_Manager_CreateTasks(void);
void App_Manager_CreateIPC(void);

#endif /* __APP_MANAGER_H */

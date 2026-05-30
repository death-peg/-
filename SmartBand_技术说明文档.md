# SmartBand 智能健康监测手环 — 技术说明文档

> **版本**: v1.0  
> **日期**: 2026-05-30  
> **主控**: STM32L431RCT6 (Cortex-M4F, 80MHz)  
> **RTOS**: FreeRTOS Kernel V10.x + CMSIS-RTOS2 API  
> **编译器**: ARMCC V5.06 (Keil MDK 5.24)

---

## 目录

1. [项目概述](#1-项目概述)
2. [硬件平台与引脚分配](#2-硬件平台与引脚分配)
3. [软件架构层次](#3-软件架构层次)
4. [FreeRTOS 任务调度体系](#4-freertos-任务调度体系)
5. [IPC 通信机制详解](#5-ipc-通信机制详解)
6. [各任务详细设计](#6-各任务详细设计)
7. [数据流全景图](#7-数据流全景图)
8. [算法模块](#8-算法模块)
9. [通信协议](#9-通信协议)
10. [构建系统](#10-构建系统)
11. [技术栈总览](#11-技术栈总览)
12. [架构优势分析](#12-架构优势分析)

---

## 1. 项目概述

SmartBand 是一款基于 STM32L4 平台、具备 **4G 全国范围远程报警** 能力的智能健康监测手环。系统通过多种生物传感器实时采集用户的生理数据，在检测到异常（心率过高/过低、血压超标、跌倒撞击）时，通过合宙 Air780E 4G Cat.1 模块经 MQTT 协议将报警信息推送至云服务器，最终送达监护人手机 App。

### 核心功能

| 功能 | 传感器 | 采样频率 | 实现方式 |
|---|---|---|---|
| 心率监测 | MAX30102 (PPG) | 100Hz | 时域峰值检测算法 |
| 血压估算 | MAX30102 (PPG特征) | 1Hz | PTT + 灌注指数无袖带估算 |
| 撞击/跌倒检测 | ADXL345 (加速度计) | 200Hz | 急动度 + 姿态角变化 |
| 秒表计时 | TIM2 硬件定时 | 100Hz | 按键控制启动/停止/分段 |
| 4G 远程报警 | Air780E Cat.1 | 事件驱动 | MQTT → 云端 → 监护人 |
| OLED 显示 | SSD1306 128×64 | 10Hz | I2C 驱动实时刷新 |
| 触觉反馈 | 振动马达 | — | 多模式振动 |

---

## 2. 硬件平台与引脚分配

### 2.1 主控芯片

| 参数 | 值 |
|---|---|
| 型号 | STM32L431RCT6 |
| 内核 | ARM Cortex-M4F (带 FPU) |
| 主频 | 80MHz (HSE 12MHz × PLL) |
| Flash | 256KB |
| SRAM | 64KB |
| 封装 | LQFP-64 |

### 2.2 时钟树

```
HSE (12MHz)
  └─ /PLLM(3) → 4MHz
       └─ ×PLLN(40) → 160MHz
            ├─ /PLLR(2) → 80MHz (SYSCLK)
            ├─ /PLLQ(2) → 80MHz (48MHz 时钟源)
            └─ /PLLP(7) → (未用)

HCLK = 80MHz, APB1 = 80MHz, APB2 = 80MHz
```

### 2.3 外设引脚全表

| STM32 引脚 | 外设总线 | 外设 | 功能说明 |
|---|---|---|---|
| PB6 | I2C1_SCL | MAX30102 | 心率传感器时钟 |
| PB7 | I2C1_SDA | MAX30102 | 心率传感器数据 |
| PB1 | GPIO | MAX30102 | 中断引脚 (FIFO Almost Full) |
| PB10 | I2C2_SCL | SSD1306 OLED | 显示屏时钟 |
| PB11 | I2C2_SDA | SSD1306 OLED | 显示屏数据 |
| PA4 | GPIO | ADXL345 | SPI 片选 (CS) |
| PA5 | SPI1_SCK | ADXL345 | 加速度计时钟 |
| PA6 | SPI1_MISO | ADXL345 | 加速度计数据输入 |
| PA7 | SPI1_MOSI | ADXL345 | 加速度计数据输出 |
| PA8 | GPIO | ADXL345 | 中断引脚 INT1 |
| PB12 | GPIO | nRF24L01+ | SPI 片选 (CSN) |
| PB13 | SPI2_SCK | nRF24L01+ | 无线模块时钟 |
| PB14 | SPI2_MISO | nRF24L01+ | 无线模块数据输入 |
| PB15 | SPI2_MOSI | nRF24L01+ | 无线模块数据输出 |
| PA3 | GPIO | nRF24L01+ | 使能引脚 (CE) |
| PC10 | UART3_TX | Air780E | 4G 模块发送 ⚠需电平转换 |
| PC11 | UART3_RX | Air780E | 4G 模块接收 ⚠需电平转换 |
| PC13 | GPIO | Air780E | PWRKEY (开机控制) |
| PC14 | GPIO | Air780E | RESET |
| PC15 | GPIO | Air780E | NET_STATUS 指示灯 |
| PA9 | UART1_TX | 调试串口 | USB-TTL 输出 |
| PA10 | UART1_RX | 调试串口 | USB-TTL 输入 |
| PA0 | GPIO | 按键 | MODE 键 (下拉触发) |
| PA1 | GPIO | 按键 | SELECT 键 (下拉触发) |
| PA2 | GPIO | 振动马达 | NMOS 驱动 |

### 2.4 外设总线拓扑

```
                 STM32L431RCT6
                      │
        ┌─────────────┼─────────────┐
        │              │              │
      I2C1           I2C2           SPI1
     (400kHz)       (400kHz)       (5MHz)
        │              │              │
    MAX30102      SSD1306        ADXL345
    (心率血氧)    (128x64 OLED)   (加速度计)
        │
      SPI2           UART3          UART1
     (10MHz)       (115200)       (115200)
        │              │              │
   nRF24L01+      Air780E        调试串口
   (2.4GHz无线)   (4G Cat.1)     (USB-TTL)
```

---

## 3. 软件架构层次

项目采用 **分层架构**，自底向上分为 5 层：

```
┌─────────────────────────────────────────────┐
│               应用层 (App/)                  │
│  7 个 FreeRTOS 任务 + 报警/显示逻辑          │
├─────────────────────────────────────────────┤
│              中间件层 (Middleware/)          │
│  FreeRTOS │ 心率算法 │ 血压算法 │ 撞击检测   │
├─────────────────────────────────────────────┤
│              协议层 (Protocol/)              │
│  报警帧编解码 │ JSON 序列化 │ CRC-8 校验     │
├─────────────────────────────────────────────┤
│              硬件抽象层 (HW_Drivers/)        │
│  MAX30102 │ ADXL345 │ SSD1306 │ Air780E     │
│  nRF24L01+ │ Vibrator                       │
├─────────────────────────────────────────────┤
│              HAL 驱动层 (Drivers/)           │
│  STM32L4xx HAL │ CMSIS Core │ 启动文件       │
└─────────────────────────────────────────────┘
```

### 3.1 目录结构详解

```
SmartBand/
├── Core/                         # STM32 核心代码
│   ├── Inc/
│   │   ├── main.h                # 引脚宏、外设句柄声明、系统时钟声明
│   │   ├── stm32l4xx_hal_conf.h  # HAL 模块裁剪配置
│   │   └── stm32l4xx_it.h        # 中断服务函数声明
│   └── Src/
│       ├── main.c                # 主入口: 时钟+外设初始化, 启动调度器
│       ├── stm32l4xx_it.c        # 中断服务实现 (SysTick/I2C/SPI/UART等)
│       └── system_stm32l4xx.c    # 系统时钟变量+MSI频率表
│
├── App/                          # FreeRTOS 应用层
│   ├── Inc/
│   │   ├── app_manager.h         # IPC定义, 数据结构, 阈值, MQTT配置
│   │   └── task_*.h              # 各任务头文件
│   └── Src/
│       ├── app_manager.c         # IPC创建+任务创建+优先级定义
│       ├── task_heartrate.c      # 心率监测 (100Hz, 优先级4)
│       ├── task_impact.c         # 撞击检测 (200Hz, 优先级5)
│       ├── task_alert.c          # 4G 报警发送 (事件驱动, 优先级3)
│       ├── task_bp.c             # 血压估算 (1Hz, 优先级2)
│       ├── task_stopwatch.c      # 秒表计时 (100Hz, 优先级2)
│       ├── task_display.c        # OLED 刷新 (10Hz, 优先级1)
│       └── task_sysmon.c         # 系统监控 (1Hz, 优先级0)
│
├── HW_Drivers/                   # 硬件驱动封装
│   ├── Inc/hw_*.h                # 6 个外设驱动接口
│   └── Src/hw_*.c                # MAX30102/ADXL345/SSD1306/Air780E/nRF24L01+/Vibrator
│
├── Middleware/
│   ├── FreeRTOS/                 # FreeRTOS V10.x 内核
│   │   ├── Inc/
│   │   │   ├── FreeRTOSConfig.h  # 内核配置 (调度/内存/优先级等)
│   │   │   ├── portable/portmacro.h  # ARMCC v5 移植宏
│   │   │   └── *.h               # 标准 FreeRTOS 头文件
│   │   └── Src/
│   │       ├── portable/port.c   # ARMCC v5 移植 (上下文切换汇编)
│   │       ├── cmsis_os2.c       # CMSIS-RTOS2 → FreeRTOS 适配层
│   │       └── *.c               # 标准 FreeRTOS 源文件
│   ├── Algorithm/                # 信号处理算法
│   │   ├── algo_heartrate.*      # PPG 时域峰值检测心率
│   │   ├── algo_bp_estimation.*  # PTT 无袖带血压估算
│   │   └── algo_impact_detect.*  # 急动度+姿态角 撞击/跌倒
│   └── Protocol/                 # 通信协议
│       ├── proto_alert.h         # 报警帧结构 (24B 固定帧+CRC8)
│       └── proto_alert.c         # 帧编解码实现
│
├── Drivers/                      # STM32 HAL + CMSIS
│   ├── CMSIS/
│   │   ├── Include/core_cm4.h    # Cortex-M4 核心外设定义
│   │   └── Device/ST/STM32L4xx/  # STM32L431 设备头+启动文件
│   └── STM32L4xx_HAL_Driver/     # HAL 库源码
│
├── MDK-ARM/                      # Keil MDK 工程
│   ├── SmartBand.uvprojx         # 工程文件
│   └── Objects/                  # 编译输出
│
└── CMakeLists.txt                # CMake 构建配置 (备用)
```

---

## 4. FreeRTOS 任务调度体系

### 4.1 任务优先级与资源配置

| 任务 | 函数入口 | 优先级 | 栈大小 | 周期 | 核心职责 |
|---|---|---|---|---|---|
| 撞击检测 | `Task_ImpactDetect` | **5 (最高)** | 512 words | 5ms (200Hz) | ADXL345 读取 → 撞击/跌倒判断 → 报警 |
| 心率监测 | `Task_HeartRate` | 4 | 512 words | 10ms (100Hz) | MAX30102 FIFO → PPG 滤波 → 心率计算 |
| 报警发送 | `Task_AlertSend` | 3 | 512 words | 事件驱动 | Air780E 4G MQTT → 云推送 |
| 秒表计时 | `Task_Stopwatch` | 2 | 256 words | 10ms (100Hz) | 按键扫描 → 计时状态机 |
| 血压估算 | `Task_BloodPressure` | 2 | 384 words | 1s (1Hz) | PPG 特征提取 → 血压回归估算 |
| 显示刷新 | `Task_Display` | 1 | 384 words | 100ms (10Hz) | OLED 128×64 页面刷新 |
| 系统监控 | `Task_SystemMonitor` | **0 (最低)** | 256 words | 1s (1Hz) | 传感器检测/堆栈监控/心跳输出 |

### 4.2 优先级设计原则

```
多传感器多任务的实时响应优先级：
  撞击检测 (5) > 心率采样 (4) > 4G 报警 (3) > 秒表/血压 (2) > 显示刷新 (1) > 系统监控 (0)
```

**设计考量**：
- **撞击检测最高优先级**：跌倒/撞击属于紧急安全事件，延迟容忍度最小(5ms)
- **心率次高**：100Hz 高频采样需要稳定的时序，不可被长时间阻塞
- **4G 报警中优先级**：虽然是关键功能，但 Air780E AT 指令交互耗时较长，设为中优先级避免饿死高实时任务
- **血压与秒表同优先级**：血压 1Hz 低频，秒表 100Hz 但对精度要求不高，二者时间片轮转
- **显示最低实用优先级**：10Hz 刷新，人眼视觉暂留 100ms 容忍延迟

### 4.3 FreeRTOS 内核配置

```
configUSE_PREEMPTION          = 1     # 抢占式调度
configTICK_RATE_HZ            = 1000  # 1ms 系统滴答
configMAX_PRIORITIES          = 7     # 0~6 共 7 级
configTOTAL_HEAP_SIZE         = 24KB  # 动态内存堆
configMINIMAL_STACK_SIZE      = 128 words

configUSE_MUTEXES             = 1     # 互斥信号量 (总线保护)
configUSE_COUNTING_SEMAPHORES = 1     # 计数信号量
configUSE_TIMERS              = 1     # 软件定时器
configCHECK_FOR_STACK_OVERFLOW = 2    # 栈溢出检测

configUSE_MALLOC_FAILED_HOOK  = 1     # 内存不足回调
configUSE_TICKLESS_IDLE       = 1     # 低功耗 Tickless
```

---

## 5. IPC 通信机制详解

### 5.1 全局 IPC 对象总览

```
┌──────────────────────────────────────────────────────────┐
│                      IPC 通信架构                         │
├──────────────┬───────────────┬───────────────────────────┤
│     对象      │      类型      │          用途             │
├──────────────┼───────────────┼───────────────────────────┤
│ queue_ppg_raw│ Queue(32)     │ MAX30102 → 心率/血压算法   │
│ queue_hr_res │ Queue(8)      │ 心率任务 → 报警/显示       │
│ queue_bp_res │ Queue(8)      │ 血压任务 → 报警/显示       │
│ queue_accel  │ Queue(16)     │ 撞击任务 → 报警/显示       │
│ queue_alert  │ Queue(16)     │ 任意任务 → 4G 报警发送     │
│ queue_disp   │ Queue(4)      │ 各任务 → OLED 显示刷新     │
│ sem_i2c1     │ Mutex         │ I2C1 总线保护 (MAX30102)   │
│ sem_i2c2     │ Mutex         │ I2C2 总线保护 (OLED)       │
│ sem_spi1     │ Mutex         │ SPI1 总线保护 (ADXL345)    │
│ event_alert  │ EventGroup    │ 报警状态位集合              │
└──────────────┴───────────────┴───────────────────────────┘
```

### 5.2 队列通信链路

```
MAX30102 ──FIFO读取──▶ PPG_RawData
                          │
              ┌───────────┤
              ▼           ▼
         queue_ppg_raw  心率算法(本地)
         (32深度)           │
              │           ▼
              │        HR_Result
              │           │
              ├───── queue_hr_result ────▶ 报警任务
              │                           │
              ▼                           ▼
         血压任务(1Hz)              queue_alert_event
              │                     (16深度, 事件驱动)
              ▼                           │
         BP_Result                  ┌─────┴─────┐
              │                     ▼           ▼
         queue_bp_result      Air780E MQTT   OLED显示
                             (4G全国推送)   (本地显示)
```

### 5.3 互斥锁策略

三个外设总线各配置一个互斥信号量，防止多任务并发访问造成总线冲突：

```
心率任务 ──┐
血压任务 ──┤ xSemaphoreTake(sem_i2c1_mutex) → I2C1 操作 → xSemaphoreGive
           │
显示任务 ──┤ xSemaphoreTake(sem_i2c2_mutex) → I2C2 操作 → xSemaphoreGive
           │
撞击任务 ──┤ xSemaphoreTake(sem_spi1_mutex) → SPI1 操作 → xSemaphoreGive
```

所有 `xSemaphoreTake` 均设置超时（3~200ms），防止死锁。

### 5.4 事件组

```c
#define EVENT_BIT_HR_ALERT       (1 << 0)   // 心率报警
#define EVENT_BIT_BP_ALERT       (1 << 1)   // 血压报警
#define EVENT_BIT_IMPACT_ALERT   (1 << 2)   // 撞击报警
#define EVENT_BIT_LOW_BATTERY    (1 << 3)   // 低电量
#define EVENT_BIT_DEVICE_OFFLINE (1 << 4)   // 设备离线
#define EVENT_BIT_ALL_ALERTS     (0x1F)     // 所有报警掩码
```

事件组用于报警状态的整体监控，可被多个任务同时等待 (`xEventGroupWaitBits`)。

---

## 6. 各任务详细设计

### 6.1 Task_ImpactDetect（撞击检测，200Hz，优先级 5）

```
流程:
  1. 获取 SPI1 互斥锁 → 初始化 ADXL345 (±16g, 200Hz)
  2. 初始化撞击检测算法
  3. 循环 (vTaskDelayUntil, 5ms 精确周期):
     a. 获取 SPI1 互斥锁 → 读取三轴原始值
     b. 转换为 g 值 (0.004g/LSB)
     c. 计算合加速度 |a| = √(ax²+ay²+az²)
     d. 调用撞击算法判断
     e. |a| > 8g → 发送 CRITICAL 级别报警 (疑似跌倒)
     f. |a| > 5g → 发送 DANGER 级别报警
     g. |a| > 2g → 发送 WARNING 级别报警
     h. 触发振动马达
     i. 推送至 queue_accel_data
```

### 6.2 Task_HeartRate（心率监测，100Hz，优先级 4）

```
流程:
  1. 获取 I2C1 互斥锁 → 初始化 MAX30102 (心率模式, 100Hz)
  2. 初始化心率算法 (动态阈值)
  3. 循环 (vTaskDelayUntil, 10ms 周期):
     a. 获取 I2C1 互斥锁 → 读 FIFO sample count
     b. while (fifo_count > 0):
        - 读取 IR + Red 通道值
        - 推送至 queue_ppg_raw (供血压任务使用)
        - 调用 Algo_HR_Process(ir_raw) 计算 BPM
        - 若 BPM > 0:
          · HR > 150 或 < 40 → DANGER 报警 → queue_alert_event
          · HR > 120 → WARNING 报警 → queue_alert_event
          · 推送至 queue_hr_result
          · 更新 queue_display_data
```

### 6.3 Task_AlertSend（报警发送，事件驱动，优先级 3）

```
流程:
  1. 初始化 Air780E (AT 指令序列: 开机/注册网络/激活 PDP/连接 MQTT)
  2. 循环:
     a. 阻塞等待 queue_alert_event (portMAX_DELAY)
     b. 检查 Air780E 状态:
        - 若离线 → 自动重连 (Air780E_AutoInit)
        - 若在线 → 构建 JSON → MQTT Publish
     c. 本地振动反馈
  3. 每 30s 发送心跳包 {"st":"online","csq":信号强度}
```

**MQTT 数据格式**：
```json
{"type":1,"level":3,"val":160,"msg":"HR 160 DANGER","ts":12345678}
```

### 6.4 Task_BloodPressure（血压估算，1Hz，优先级 2）

```
流程:
  1. 初始化血压估算算法 (默认校准值 120/80)
  2. 循环 (vTaskDelayUntil, 1s 周期):
     a. 1 秒内从 queue_ppg_raw 收集 20 个 PPG 样本
     b. 若样本 ≥ 10 个 → 调用 Algo_BP_Estimate()
     c. 若置信度 > 30%:
        - 推送至 queue_bp_result
        - 收缩压 > 180 或 舒张压 > 120 → DANGER 报警 → queue_alert_event
        - 收缩压 > 140 或 舒张压 > 90 → WARNING 报警
```

**算法原理**：基于 Moens-Korteweg 方程简化
```
SBP ≈ a - b × ln(PTT)
DBP ≈ c - d × ln(PTT)
```
其中 PTT (Pulse Transit Time) 从 PPG 波形特征点提取。

### 6.5 Task_Stopwatch（秒表计时，100Hz，优先级 2）

```
流程:
  1. 初始化 TIM2 硬件定时器
  2. 循环 (vTaskDelayUntil, 10ms 周期):
     a. 扫描 PA0(MODE键):
        - 短按: 启动/停止计时
        - 长按(>1s): 复位归零
     b. 扫描 PA1(SELECT键):
        - 短按: 记录分段 (最多 10 段)
     c. 更新 queue_display_data
```

### 6.6 Task_Display（OLED 刷新，10Hz，优先级 1）

```
流程:
  1. 获取 I2C2 互斥锁 → 初始化 SSD1306 OLED
  2. 循环 (vTaskDelayUntil, 100ms 周期):
     a. 非阻塞获取 queue_display_data 最新数据
     b. 获取 I2C2 互斥锁 → 刷新屏幕:
        行1: "HR: 72 BPM"
        行2: "BP: 120/80"
        行3: "STOP: 00:05.23"
        行4: "G: 1.02"
        行5: "BAT: 85%"
```

### 6.7 Task_SystemMonitor（系统监控，1Hz，优先级 0）

```
流程:
  1. 上电检测传感器 ID (MAX30102=0x15, ADXL345=0xE5)
  2. 输出启动信息到调试串口
  3. 循环 (vTaskDelayUntil, 1s 周期):
     a. 每秒输出心跳到串口
     b. 每 10s 检查各任务栈水位 (uxTaskGetStackHighWaterMark)
     c. 模拟电池电量 (待接入 ADC)
```

---

## 7. 数据流全景图

```
                       ┌──────────────┐
                       │  MAX30102    │
                       │  (I2C1, PB6-7)│
                       └──────┬───────┘
                              │ PPG 原始数据 (100Hz)
                              ▼
                    ┌─────────────────┐
                    │  Task_HeartRate  │ 优先级 4, 100Hz
                    │  心率采样+算法   │
                    └───┬─────────┬───┘
                        │         │
           queue_ppg_raw│         │ queue_hr_result
                        ▼         ▼
              ┌─────────────┐  ┌──────────────┐
              │ Task_BP     │  │ Task_Alert    │
              │ 血压估算 1Hz │  │ 4G MQTT 发送  │
              └──┬──────────┘  └──┬───┬───────┘
                 │                │   │
     queue_bp_result              │   │ queue_alert_event
                 │                │   │
                 ▼                ▼   ▼
              ┌──────────────────────────┐
              │      Task_Display        │
              │      OLED 刷新 10Hz       │
              └──────────────────────────┘
                        ▲
           queue_display_data
                        │
     ┌──────────────────┴──────────────────┐
     │                                     │
┌────┴───────┐  ┌──────────────┐  ┌───────┴──────┐
│ Task_Impact│  │Task_Stopwatch│  │Task_SysMon   │
│ 撞击 200Hz │  │ 秒表 100Hz   │  │ 监控 1Hz     │
└────────────┘  └──────────────┘  └──────────────┘
     │
 ADXL345 (SPI1, PA4-7)
```

---

## 8. 算法模块

### 8.1 心率算法 (`algo_heartrate`)

| 参数 | 值 |
|---|---|
| 方法 | 时域峰值检测 (Peak Detection) |
| 输入 | MAX30102 IR 通道原始值 |
| 采样率 | 100Hz |
| 滑动窗口 | 256 样本 (2.56s) |
| 峰间最小间隔 | 30 样本 (300ms, 对应最大心率 200 BPM) |
| 动态阈值因子 | 60% (自适应) |
| 输出 | BPM (0=未检出) + 置信度 0~100 |

### 8.2 血压估算算法 (`algo_bp_estimation`)

| 参数 | 值 |
|---|---|
| 方法 | PTT (脉搏传导时间) + PPG 波形特征 |
| 输入 | 20 个 PPG_RawData_t 样本 |
| 校准 | 默认 120/80, 支持用户校准 (3次读数) |
| 输出 | 收缩压/舒张压 (mmHg) + 置信度 |

### 8.3 撞击检测算法 (`algo_impact_detect`)

| 参数 | 值 |
|---|---|
| 方法 | 合加速度 + 急动度(Jerk) + 姿态角变化 |
| 输入 | ADXL345 三轴加速度 (200Hz) |
| 撞击阈值 | 合加速度 > 2g 触发 |
| 急动度阈值 | 3.0 g/(10ms) |
| 检测窗口 | 10 样本 (50ms) |
| 跌倒姿态角 | 45° 变化阈值 |
| 输出 | is_impact / is_fall 标志 |

---

## 9. 通信协议

### 9.1 报警帧协议（本地 nRF24L01+ 无线）

24 字节固定帧，带 CRC-8 校验：

```
Offset  Size  Field        Description
─────────────────────────────────────────
0       2     sync          同步字 0xAA 0x55
2       2     device_id     设备 ID (大端)
4       1     alert_type    报警类型
5       1     alert_level   报警级别
6       2     value         报警值 (大端)
8       4     timestamp     时间戳 ms (大端)
12      4     reserved      保留
16      8     message       报警描述 (ASCII)
24      1     crc8          CRC-8 校验
─────────────────────────────────────────
Total   26 bytes
```

### 9.2 MQTT 报警协议（4G 远程）

```json
{
  "type": 1,           // 0x01=心率, 0x02=血压, 0x04=撞击, 0x08=低电量
  "level": 3,          // 0=无, 1=警告, 2=危险, 3=危急
  "val": 160,           // 报警相关数值
  "msg": "HR 160 DANGER!",
  "ts": 12345678        // 毫秒时间戳
}
```

MQTT Topic 配置：
- 发布主题: `smartband/alert`
- 心跳主题: `smartband/heartbeat`
- Broker: EMQX 公共测试 (生产换阿里云 IoT)
- Keepalive: 60s

---

## 10. 构建系统

### 10.1 编译器配置

| 项目 | 值 |
|---|---|
| IDE | Keil MDK V5.24.2.0 |
| 编译器 | Armcc.exe V5.06 update 5 (build 528) |
| 汇编器 | Armasm.exe V5.06 update 5 |
| 链接器 | ArmLink.exe V5.06 update 5 |
| 设备包 | Keil.STM32L4xx_DFP.1.4.0 |
| CPU DLL | SARMCM3.DLL |
| 输出 | SmartBand.axf (ELF) |

### 10.2 编译依赖关系

```
stm32l4xx_hal_conf.h ──启用模块──▶ HAL 驱动源码
main.h ──引脚+外设定义──▶ 所有应用层代码
FreeRTOSConfig.h ──内核参数──▶ FreeRTOS 内核
portmacro.h + port.c ──ARMCC v5 移植──▶ 上下文切换汇编
```

### 10.3 HAL 模块裁剪

仅启用 12 个必要模块，减小固件大小：
```
HAL_MODULE_ENABLED      # HAL 核心
HAL_CORTEX_MODULE_ENABLED  # Cortex-M4 系统
HAL_DMA_MODULE_ENABLED     # DMA
HAL_GPIO_MODULE_ENABLED    # GPIO
HAL_RCC_MODULE_ENABLED     # 时钟
HAL_PWR_MODULE_ENABLED     # 电源
HAL_FLASH_MODULE_ENABLED   # Flash
HAL_I2C_MODULE_ENABLED     # I2C (MAX30102 + OLED)
HAL_SPI_MODULE_ENABLED     # SPI (ADXL345 + nRF24L01+)
HAL_UART_MODULE_ENABLED    # UART (调试 + Air780E)
HAL_TIM_MODULE_ENABLED     # 定时器 (秒表)
HAL_RTC_MODULE_ENABLED     # RTC
HAL_EXTI_MODULE_ENABLED    # 外部中断
```

---

## 11. 技术栈总览

| 层级 | 技术 | 用途 |
|---|---|---|
| **MCU 平台** | STM32L431RCT6 | Cortex-M4F 低功耗主控 |
| **RTOS** | FreeRTOS V10.x | 抢占式实时多任务调度 |
| **RTOS API** | CMSIS-RTOS2 | 标准化 RTOS 接口 |
| **HAL 库** | STM32L4xx HAL | 外设驱动抽象 |
| **编译器** | ARMCC V5.06 | Keil MDK 工具链 |
| **传感器** | MAX30102 | PPG 心率/血氧 |
| **加速度计** | ADXL345 | 3 轴 ±16g 撞击检测 |
| **显示屏** | SSD1306 | 128×64 OLED I2C |
| **4G 通信** | 合宙 Air780E | Cat.1 全网通 MQTT |
| **无线** | nRF24L01+ | 2.4GHz 本地报警 |
| **协议** | MQTT v3.1.1 | IoT 消息推送 |
| **心跳算法** | Peak Detection | 时域动态阈值 |
| **血压算法** | PTT Estimation | Moens-Korteweg 简化 |
| **撞击算法** | Jerk + Attitude | 急动度 + 姿态角 |
| **校验** | CRC-8 | 报警帧完整性 |
| **内存管理** | heap_4 | FreeRTOS 动态分配 |

---

## 12. 架构优势分析

### 12.1 实时性保障

- **抢占式调度**：高优先级任务可立即抢占 CPU，撞击检测 5ms 硬实时
- **vTaskDelayUntil** 精确周期：所有周期性任务使用绝对延时，无累积漂移
- **总线互斥**：I2C/SPI 共享总线使用超时互斥锁，防死锁
- **中断安全**：BASEPRI 分级屏蔽，ISR 中仅使用 FromISR API

### 12.2 解耦设计

- **消息队列解耦**：生产者-消费者模式，各任务通过队列异步通信，互不阻塞
- **分层架构**：硬件驱动 → 算法 → 协议 → 应用，每层独立可替换
- **CMSIS-RTOS2 适配层**：应用代码可无缝迁移至其他 RTOS（只需替换 cmsis_os2.c）

### 12.3 低功耗设计

- **Tickless Idle**：无任务时进入 STOP 模式，SysTick 由 LPTIM 替代
- **外设按需使能**：传感器仅在对应任务运行时上电
- **DMA 传输**：SPI/I2C 批量数据使用 DMA 减少 CPU 唤醒

### 12.4 可靠性设计

- **栈溢出检测**：`configCHECK_FOR_STACK_OVERFLOW = 2`，运行时监控
- **内存分配失败 Hook**：`vApplicationMallocFailedHook` 捕获 OOM
- **传感器 ID 验证**：启动时读取 MAX30102 PartID (0x15) 和 ADXL345 DevID (0xE5)
- **MQTT 断线重连**：Air780E 状态机自动检测网络断开并重建连接
- **CRC-8 校验**：报警帧带完整性校验，防止误报
- **事件组冗余**：关键报警同时设置事件位，支持多任务联合响应

### 12.5 可扩展性

- **模块化驱动**：新增传感器只需添加 `hw_xxx.c/h` + 配套任务
- **优先级可配**：7 级优先级灵活分配，可插入新任务
- **队列深度可调**：每个队列独立配置缓冲大小
- **算法可替换**：`algo_*` 模块接口标准化，可替换为更高级算法（如深度学习推理）

---

## 附录 A：编译与烧录

### 前置条件
- Keil MDK V5.24+ 已安装
- STM32L4xx DFP 1.4.0 已安装
- ST-Link V2 调试器

### 编译步骤
1. 打开 `MDK-ARM/SmartBand.uvprojx`
2. 确认 Project → Options → Target → ARM Compiler 选择 "V5.06"
3. 确认 C/C++ 选项卡 Include Paths 包含所有 Inc 目录
4. F7 编译 (Build)
5. F8 下载 (Download)

### 预期输出
- 编译结果: 0 Error(s), 0 Warning(s)  (除 1 个 A1581W 对齐填充警告可忽略)
- 固件大小: 约 40~60KB
- 调试串口 (UART1, 115200 8N1) 输出启动信息

---

## 附录 B：安全阈值配置汇总

| 参数 | WARNING | DANGER |
|---|---|---|
| 心率过高 | > 120 BPM | > 150 BPM |
| 心率过低 | — | < 40 BPM |
| 收缩压过高 | > 140 mmHg | > 180 mmHg |
| 舒张压过高 | > 90 mmHg | > 120 mmHg |
| 轻度撞击 | > 2g | — |
| 严重撞击 | — | > 5g |
| 疑似跌倒 | — | > 8g |

---

> **声明**: 血压估算为无袖带方法，精度有限，仅供健康趋势参考，不可替代医用血压计。

---

*文档生成时间: 2026-05-30 | SmartBand Project*

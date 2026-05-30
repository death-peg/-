# 🏥 SmartBand — 智能健康监测手环 (4G 全国报警版)

基于 **STM32L431RCT6** + **FreeRTOS** + **合宙 Air780E 4G Cat.1** 的远程健康监测可穿戴设备。

> 📡 全国范围报警 · MQTT 云推送 · 嘉立创可打板 · BOM ≈ ¥79

---

## 📋 功能特性

| 功能 | 硬件 | 频率 | 说明 |
|---|---|---|---|
| ❤️ 心率监测 | MAX30102 (I2C1) | 100Hz | PPG 动态阈值峰值检测 |
| 🩸 血压估算 | MAX30102 + 算法 | 1Hz | 灌注指数 PI 无袖带估算 |
| 💥 撞击/跌倒 | ADXL345 (SPI1) | 200Hz | 急动度 + 姿态角变化 |
| ⏱️ 秒表计时 | TIM2 硬件定时 | 100Hz | 启动/停止/Lap/长按复位 |
| 📡 **4G 远程报警** | **Air780E (UART3)** | 事件驱动 | **MQTT → 云 → 监护人 App** |
| 🖥️ OLED 显示 | SSD1306 (I2C2) | 10Hz | 128×64 实时数据 |
| 📳 触觉反馈 | 振动马达 (GPIO) | — | 多模式 (短振/长振/SOS) |

---

## 📁 项目架构

```
SmartBand/
├── Core/                    # STM32 核心 (main/中断/时钟)
├── App/                     # FreeRTOS 应用层 (7 个 Task)
├── HW_Drivers/              # 硬件驱动
│   ├── hw_max30102.c        #   心率传感器 (I2C)
│   ├── hw_adxl345.c         #   加速度计 (SPI)
│   ├── hw_oled_ssd1306.c    #   OLED 显示 (I2C)
│   ├── hw_air780e.c         # ★ 4G 模块 AT+MQTT (UART)
│   └── hw_vibrator.c        #   振动马达 (GPIO)
├── Middleware/
│   ├── FreeRTOS/            #   FreeRTOSConfig.h
│   ├── Algorithm/           #   心率/血压/撞击算法
│   └── Protocol/            #   报警帧协议 (CRC-8)
├── Hardware/                 # ★ 嘉立创 PCB 设计
│   ├── PCB_Design_Guide.md  #   原理图 + 布局指南
│   └── BOM.csv              #   物料清单
├── MDK-ARM/                 # Keil 工程
└── README.md
```

---

## ⚡ 硬件连接

| STM32 引脚 | 外设 | 备注 |
|---|---|---|
| PB6/PB7 | MAX30102 I2C1 | 心率血氧 |
| PB10/PB11 | SSD1306 I2C2 | OLED |
| PA4~PA7 | ADXL345 SPI1 | 加速度 |
| **PC10/PC11** | **Air780E UART3** | **⚠ 需 1.8V↔3.3V 电平转换** |
| PC13~PC15 | Air780E 控制 | PWRKEY/RESET/NET |
| PA0/PA1 | 按键 MODE/SELECT | |
| PA2 | 振动马达 (NMOS) | |
| PA9/PA10 | 调试串口 | |

---

## 🌐 4G 报警链路

```
手环检测异常 ──UART3──▶ Air780E ──4G LTE──▶ 基站
                                               │
                    监护人手机 ◀──推送── 云MQTT服务器
                    
心跳包: 每30s发送 {"st":"online","csq":28}
报警包: {"type":1,"level":3,"val":160,"msg":"HR 160 DANGER"}
```

---

## 🚀 快速上手

### 1. STM32CubeMX 生成基础工程
- 选择 `STM32L431RCTx`
- 按 `Core/Inc/main.h` 引脚定义配置 I2C/SPI/UART
- Middleware → FREERTOS → CMSIS_V2
- 生成后复制 `Drivers/CMSIS` 和 `Drivers/STM32L4xx_HAL_Driver`

### 2. 下载 FreeRTOS 内核源码
- 从 CubeMX 生成的工程或 FreeRTOS 官网获取 `.c` 文件
- 放入 `Middleware/FreeRTOS/Src/`

### 3. 嘉立创打板
- 参照 `Hardware/PCB_Design_Guide.md`
- 或直接在嘉立创 EDA 中按连接表绘制原理图
- 板尺寸建议 40mm×28mm

### 4. 编译 & 烧录
- 打开 `MDK-ARM/SmartBand.uvprojx`
- 检查 Include Paths
- F7 编译, F8 下载

---

## 📦 BOM

| 核心物料 | 型号 | 单价 |
|---|---|---|
| MCU | STM32L431RCT6 | ¥18 |
| **4G 模块** | **合宙 Air780E** | **¥16** |
| 心率 | MAX30102 模块 | ¥12 |
| 加速度 | ADXL345 模块 | ¥6 |
| OLED | SSD1306 128×64 | ¥8 |
| eSIM | 贴片物联网卡 | ¥2 |
| 电池 | 3.7V 350mAh | ¥10 |
| 其他 | PCB+阻容+马达+按键 | ¥7 |
| **合计** | | **≈¥79** |

详见 `Hardware/BOM.csv`

---

## ⚠️ 关键注意事项

1. **电平转换必做**：Air780E UART 是 1.8V，STM32 是 3.3V！用 BSS138 MOS 管或 TXS0104E
2. **4G 天线区域禁止铺铜**
3. **Air780E 直接电池供电**，不要经过 3.3V LDO
4. **血压精度有限**，仅供健康参考
5. **MQTT Broker**：开发用 `broker.emqx.io`，生产换阿里云 IoT

---

*STM32L431RCT6 · Air780E 4G Cat.1 · FreeRTOS CMSIS-RTOS2 · Keil MDK · 嘉立创 EDA*


---

## 📁 项目架构

```
SmartBand/
├── Core/                               # STM32 核心文件
│   ├── Inc/
│   │   ├── main.h                      # 主头文件 + 引脚定义
│   │   ├── stm32l4xx_hal_conf.h        # HAL 模块裁剪
│   │   └── stm32l4xx_it.h              # 中断声明
│   └── Src/
│       ├── main.c                      # 主入口 + 外设初始化
│       ├── stm32l4xx_it.c              # 中断服务函数
│       └── system_stm32l4xx.c          # 系统时钟
│
├── App/                                # FreeRTOS 应用任务层
│   ├── Inc/
│   │   ├── app_manager.h               # 任务管理器 + IPC 定义
│   │   ├── task_heartrate.h            # 心率任务
│   │   ├── task_impact.h               # 撞击检测任务
│   │   ├── task_alert.h                # 报警发送任务
│   │   ├── task_display.h              # 显示刷新任务
│   │   ├── task_stopwatch.h            # 秒表计时任务
│   │   ├── task_bp.h                   # 血压估算任务
│   │   └── task_sysmon.h               # 系统监控任务
│   └── Src/
│       ├── app_manager.c               # IPC 创建 + 7 个任务创建
│       ├── task_heartrate.c            # 100Hz PPG 采集 → 心率计算
│       ├── task_impact.c               # 200Hz 加速度 → 撞击/跌倒
│       ├── task_alert.c                # 事件驱动 → nRF24L01 发送
│       ├── task_display.c              # 10Hz OLED 刷新
│       ├── task_stopwatch.c            # 秒表逻辑 + 按键交互
│       ├── task_bp.c                   # 1Hz 血压特征提取
│       └── task_sysmon.c               # 传感器健康检查 + 栈监控
│
├── HW_Drivers/                         # 硬件外设驱动
│   ├── Inc/
│   │   ├── hw_max30102.h               # MAX30102 心率传感器
│   │   ├── hw_adxl345.h                # ADXL345 加速度计
│   │   ├── hw_oled_ssd1306.h           # SSD1306 OLED 显示
│   │   ├── hw_nrf24l01.h               # nRF24L01+ 无线模块
│   │   └── hw_vibrator.h               # 振动马达
│   └── Src/  (对应 .c 实现)
│
├── Middleware/                         # 中间件
│   ├── FreeRTOS/
│   │   └── Inc/
│   │       └── FreeRTOSConfig.h        # RTOS 内核配置
│   ├── Algorithm/
│   │   ├── Inc/
│   │   │   ├── algo_heartrate.h        # 心率算法 API
│   │   │   ├── algo_bp_estimation.h    # 血压估算 API
│   │   │   └── algo_impact_detect.h    # 撞击检测 API
│   │   └── Src/  (对应 .c 实现)
│   └── Protocol/
│       ├── Inc/
│       │   └── proto_alert.h           # 报警帧协议
│       └── Src/
│           └── proto_alert.c           # 帧打包/解析 + CRC-8
│
├── Drivers/                            # STM32CubeMX 生成的标准驱动
│   ├── CMSIS/                          # ← 从 CubeMX 复制
│   └── STM32L4xx_HAL_Driver/           # ← 从 CubeMX 复制
│
├── MDK-ARM/                            # Keil MDK 工程
│   ├── SmartBand.uvprojx               # 工程文件
│   └── SmartBand.uvoptx                # 工程选项
│
└── README.md                           # 本文件
```

---

## 🔧 硬件连接

| STM32L431RCT6 引脚 | 外设 | 功能 |
|---|---|---|
| PB6 (SCL), PB7 (SDA) | MAX30102 | I2C1 — 心率/血氧 |
| PB10 (SCL), PB11 (SDA) | SSD1306 OLED | I2C2 — 显示 |
| PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA4 (CS) | ADXL345 | SPI1 — 加速度计 |
| PB13 (SCK), PB14 (MISO), PB15 (MOSI), PB12 (CSN) | nRF24L01+ | SPI2 — 无线通信 |
| PA3 | nRF24L01+ CE | 模式控制 |
| PB0 | nRF24L01+ IRQ | 中断 |
| PA9 (TX), PA10 (RX) | USB-TTL | 调试串口 |
| PA0 | 按键 MODE | 模式/启动停止 |
| PA1 | 按键 SELECT | 选择/Lap |
| PA2 | 振动马达 | GPIO 输出 |
| PB1 | MAX30102 INT | 中断输入 |

---

## ⚙️ FreeRTOS 任务划分

```
优先级 高 ↑
  Task_ImpactDetect   [5]  200Hz  撞击检测 (最高优先级，保证安全)
  Task_HeartRate      [4]  100Hz  心率采样 + 计算
  Task_AlertSend      [3]  事件    报警帧发送 (nRF24L01+)
  Task_Stopwatch      [2]  100Hz  秒表计时 + 按键扫描
  Task_BloodPressure  [2]  1Hz    血压特征提取
  Task_Display        [1]  10Hz   OLED 屏幕刷新
  Task_SystemMonitor  [0]  1Hz    系统健康 + 串口日志
优先级 低 ↓
```

**IPC 通信方式**:

- **消息队列**: `queue_ppg_raw` → `queue_hr_result` → `queue_display_data`
- **互斥信号量**: `sem_i2c1_mutex`, `sem_spi1_mutex` ... (外设总线保护)
- **事件组**: `event_alert_flags` (撞击事件广播)
- **xQueueOverwrite**: 显示数据覆盖式更新 (消费者滞后只取最新)

---

## 🚀 快速上手

### 1. 前置准备

- **Keil MDK-ARM** v5.36+ (含 ARM Compiler 6)
- **STM32CubeMX** v6.x (生成 HAL 库 + CMSIS)
- STM32L4 系列 Pack: `Keil.STM32L4xx_DFP.2.6.1`

### 2. 生成 HAL 驱动

1. 启动 **STM32CubeMX**
2. 新建工程 → 选择 `STM32L431RCTx`
3. 按 `main.h` 中的引脚定义配置 I2C/SPI/UART/GPIO/TIM
4. **Middleware** → 勾选 `FREERTOS` → Interface: `CMSIS_V2`
5. 生成代码 (Project Manager → Toolchain: MDK-ARM)
6. 将生成的 `Drivers/CMSIS` 和 `Drivers/STM32L4xx_HAL_Driver` 复制到本项目对应目录

### 3. 在 Keil 中打开

1. 双击 `MDK-ARM\SmartBand.uvprojx`
2. 检查 `Options → C/C++ → Include Paths` 是否包含所有 Inc 路径
3. `Options → C/C++ → Define`: `USE_HAL_DRIVER,STM32L431xx`
4. 编译 (F7)

### 4. 烧录 & 调试

- 通过 ST-Link / J-Link 连接开发板
- 下载 (F8)
- 打开串口助手 (115200-8-N-1) 查看系统日志

---

## 🔐 报警安全阈值

| 指标 | 警告 | 危险 | 危急 |
|---|---|---|---|
| 心率 (BPM) | >120 | >150 或 <40 | — |
| 收缩压 (mmHg) | >140 | >180 | — |
| 舒张压 (mmHg) | >90 | >120 | — |
| 加速度 (g) | >2.0 | >5.0 (撞击) | >8.0 (跌倒) |

阈值可在 `app_manager.h` 中修改。

---

## 📦 BOM 物料清单

| 物料 | 型号 | 数量 | 约价 |
|---|---|---|---|
| MCU | STM32L431RCT6 | 1 | ¥18 |
| 心率传感器 | MAX30102 模块 | 1 | ¥12 |
| 加速度计 | ADXL345 模块 | 1 | ¥6 |
| OLED | SSD1306 128×64 I2C | 1 | ¥8 |
| 无线模块 | nRF24L01+ PA+LNA | 1 | ¥6 |
| 振动马达 | 1027 扁平马达 | 1 | ¥2 |
| 电池 | 3.7V 200mAh 锂聚合物 | 1 | ¥10 |
| 充电 IC | TP4056 模块 | 1 | ¥2 |
| 其他 | PCB/按键/电阻/电容 | — | ¥10 |
| **合计** | | | **≈ ¥74** |

---

## ⚠️ 注意事项

1. **血压测量精度有限**：算法基于光学 PPG 特征参数间接估算，不可替代医用血压计。建议用标准血压计校准后使用。
2. **MAX30102 佩戴要求**：传感器需紧贴皮肤，避免环境光干扰和运动伪影。
3. **FreeRTOS 内核源码**：本项目的 `Middleware/FreeRTOS/Src/` 中仅包含 `FreeRTOSConfig.h`。完整的 FreeRTOS 内核 `.c` 文件需从 CubeMX 生成或从 [FreeRTOS 官网](https://www.freertos.org) 下载。
4. **nRF24L01 接收端**：本项目仅实现手环端 TX 发送。接收端 (报警主机) 需另行开发，使用相同的 `proto_alert` 协议解析帧。
5. **低功耗优化**：当前为功能验证版本。量产需启用 Tickless 模式 + STOP 睡眠 + 传感器间歇采样。

---

## 📄 许可证

MIT License — 仅供学习和研究使用。医疗用途需满足相关法规认证。

---

*Generated on 2026-05-28 · STM32L431RCT6 · FreeRTOS CMSIS-RTOS2 · Keil MDK-ARM*

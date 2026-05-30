/**
 * @file    main.h
 * @brief   智能手环 - 主头文件
 * @author  SmartBand Project
 * @date    2026-05-28
 *
 * STM32L431RCT6 主控，FreeRTOS + HAL 库
 * 功能：心率监测、血压监测、撞击检测、秒表计时、无线报警
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 头文件包含 ========================== */
#include "stm32l4xx.h"              /* CMSIS 设备头 (内部已含 stm32l4xx_hal.h) */
#include "cmsis_os2.h"              /* FreeRTOS CMSIS-RTOS2 API                */

/* ========================== 硬件引脚定义 ========================== */

/* ------------------------- 系统时钟 ------------------------- */
#ifndef HSE_VALUE
#define HSE_VALUE                12000000U   /* 外部高速晶振 12MHz */
#endif
#ifndef HSI_VALUE
#define HSI_VALUE                16000000U   /* 内部高速晶振 16MHz */
#endif
#define SYSTEM_CLOCK             80000000U   /* PLL 倍频至 80MHz   */

/* ------------------------- I2C1: MAX30102 心率/血氧 ------------------------- */
#define MAX30102_I2C             hi2c1
#define MAX30102_I2C_SCL_PIN     GPIO_PIN_6
#define MAX30102_I2C_SCL_PORT    GPIOB
#define MAX30102_I2C_SDA_PIN     GPIO_PIN_7
#define MAX30102_I2C_SDA_PORT    GPIOB
#define MAX30102_INT_PIN         GPIO_PIN_1
#define MAX30102_INT_PORT        GPIOB
#define MAX30102_I2C_ADDR        (0xAE >> 1)  /* 7-bit address */

/* ------------------------- I2C2: SSD1306 OLED (128x64) ------------------------- */
#define OLED_I2C                 hi2c2
#define OLED_I2C_SCL_PIN         GPIO_PIN_10
#define OLED_I2C_SCL_PORT        GPIOB
#define OLED_I2C_SDA_PIN         GPIO_PIN_11
#define OLED_I2C_SDA_PORT        GPIOB
#define OLED_I2C_ADDR            (0x78 << 1)

/* ------------------------- SPI1: ADXL345 加速度计 (撞击检测) ------------------------- */
#define ADXL345_SPI              hspi1
#define ADXL345_SPI_SCK_PIN      GPIO_PIN_5
#define ADXL345_SPI_SCK_PORT     GPIOA
#define ADXL345_SPI_MISO_PIN     GPIO_PIN_6
#define ADXL345_SPI_MISO_PORT    GPIOA
#define ADXL345_SPI_MOSI_PIN     GPIO_PIN_7
#define ADXL345_SPI_MOSI_PORT    GPIOA
#define ADXL345_CS_PIN           GPIO_PIN_4
#define ADXL345_CS_PORT          GPIOA
#define ADXL345_INT1_PIN         GPIO_PIN_8
#define ADXL345_INT1_PORT        GPIOA

/* ------------------------- UART1: 调试串口 (USB-TTL) ------------------------- */
#define DEBUG_UART               huart1
#define DEBUG_UART_TX_PIN        GPIO_PIN_9
#define DEBUG_UART_TX_PORT       GPIOA
#define DEBUG_UART_RX_PIN        GPIO_PIN_10
#define DEBUG_UART_RX_PORT       GPIOA

/* ------------------------- UART3: 4G Cat.1 合宙 Air780E ------------------------- */
#define AIR780E_UART             huart3
#define AIR780E_TX_PIN           GPIO_PIN_10      /* PC10 → Air780E UART1_RX */
#define AIR780E_TX_PORT          GPIOC
#define AIR780E_RX_PIN           GPIO_PIN_11      /* PC11 → Air780E UART1_TX */
#define AIR780E_RX_PORT          GPIOC
#define AIR780E_PWRKEY_PIN       GPIO_PIN_13      /* PC13 → PWRKEY (拉低>1s开机) */
#define AIR780E_PWRKEY_PORT      GPIOC
#define AIR780E_RESET_PIN        GPIO_PIN_14      /* PC14 → RESET             */
#define AIR780E_RESET_PORT       GPIOC
#define AIR780E_NETSTAT_PIN      GPIO_PIN_15      /* PC15 → NET_STATUS 指示灯 */
#define AIR780E_NETSTAT_PORT     GPIOC

/* ------------------------- GPIO: 按键 / 振动马达 ------------------------- */
#define BTN_MODE_PIN             GPIO_PIN_0
#define BTN_MODE_PORT            GPIOA
#define BTN_SELECT_PIN           GPIO_PIN_1
#define BTN_SELECT_PORT          GPIOA
#define VIBRATOR_PIN             GPIO_PIN_2
#define VIBRATOR_PORT            GPIOA

/* ------------------------- SPI2: nRF24L01+ 2.4GHz 无线 (本地报警) ------------------------- */
#define NRF24L01_SPI             hspi2
#define NRF24L01_SPI_SCK_PIN     GPIO_PIN_13
#define NRF24L01_SPI_SCK_PORT    GPIOB
#define NRF24L01_SPI_MISO_PIN    GPIO_PIN_14
#define NRF24L01_SPI_MISO_PORT   GPIOB
#define NRF24L01_SPI_MOSI_PIN    GPIO_PIN_15
#define NRF24L01_SPI_MOSI_PORT   GPIOB
#define NRF24L01_CSN_PIN         GPIO_PIN_12
#define NRF24L01_CSN_PORT        GPIOB
#define NRF24L01_CE_PIN          GPIO_PIN_3
#define NRF24L01_CE_PORT         GPIOA

/* ------------------------- TIM2: 秒表计时基准 ------------------------- */
#define STOPWATCH_TIM            htim2

/* ========================== 全局句柄声明 ========================== */
extern I2C_HandleTypeDef   MAX30102_I2C;
extern I2C_HandleTypeDef   OLED_I2C;
extern SPI_HandleTypeDef   ADXL345_SPI;
extern SPI_HandleTypeDef   NRF24L01_SPI;
extern UART_HandleTypeDef  DEBUG_UART;
extern UART_HandleTypeDef  AIR780E_UART;
extern TIM_HandleTypeDef   STOPWATCH_TIM;

/* ========================== 函数声明 ========================== */
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);
void MX_I2C2_Init(void);
void MX_SPI1_Init(void);
void MX_SPI2_Init(void);
void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);
void MX_TIM2_Init(void);
void Air780E_GPIO_Init(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

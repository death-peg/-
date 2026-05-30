/**
 * @file    stm32l4xx_hal_conf.h
 * @brief   HAL 库模块配置文件
 *
 * 根据智能手环需求裁剪 HAL 模块，减小固件体积
 */

#ifndef __STM32L4xx_HAL_CONF_H
#define __STM32L4xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 启用的 HAL 模块 ========================== */
#define HAL_MODULE_ENABLED          /* HAL 核心           */
#define HAL_CORTEX_MODULE_ENABLED   /* Cortex-M4 系统     */
#define HAL_DMA_MODULE_ENABLED      /* DMA 控制器         */
#define HAL_GPIO_MODULE_ENABLED     /* GPIO               */
#define HAL_RCC_MODULE_ENABLED      /* 时钟控制           */
#define HAL_PWR_MODULE_ENABLED      /* 电源管理(低功耗)   */
#define HAL_FLASH_MODULE_ENABLED    /* Flash 控制器       */
#define HAL_I2C_MODULE_ENABLED      /* I2C 通信           */
#define HAL_SPI_MODULE_ENABLED      /* SPI 通信           */
#define HAL_UART_MODULE_ENABLED     /* UART 串口          */
#define HAL_TIM_MODULE_ENABLED      /* 定时器             */
#define HAL_RTC_MODULE_ENABLED      /* RTC 实时时钟       */
#define HAL_EXTI_MODULE_ENABLED     /* 外部中断           */

/* ========================== 禁用的 HAL 模块 (减小体积) ========================== */
/* #define HAL_ADC_MODULE_ENABLED   */
/* #define HAL_CAN_MODULE_ENABLED   */
/* #define HAL_CRC_MODULE_ENABLED   */
/* #define HAL_DAC_MODULE_ENABLED   */
/* #define HAL_DFSDM_MODULE_ENABLED */
/* #define HAL_SAI_MODULE_ENABLED   */
/* #define HAL_SD_MODULE_ENABLED    */
/* #define HAL_USB_MODULE_ENABLED   */

/* ========================== 系统配置 ========================== */
#define TICK_INT_PRIORITY           15U     /* SysTick 优先级 (最低)  */
#define USE_RTOS                    0U      /* HAL 版本不支持 RTOS 锁    */
#define PREFETCH_ENABLE             1U      /* 使能 Flash 预取        */
#define INSTRUCTION_CACHE_ENABLE    1U      /* 使能指令缓存           */
#define DATA_CACHE_ENABLE           1U      /* 使能数据缓存           */

/* 使用 HAL 默认配置即可，以下为按需覆盖 */
#define HSE_STARTUP_TIMEOUT         100U
#define HSI_STARTUP_TIMEOUT         5000U
#define LSE_STARTUP_TIMEOUT         5000U
#define MSI_VALUE                   4000000U /* MSI 默认 4MHz        */
#define LSE_VALUE                   32768U  /* LSE 典型 32.768kHz    */
#define LSI_VALUE                   32000U  /* LSI 典型 32kHz        */
#define HSI48_VALUE                 48000000U /* HSI48 48MHz          */
#define EXTERNAL_SAI1_CLOCK_VALUE   48000U  /* SAI1 外部时钟         */
#define VDD_VALUE                   3300U   /* 供电电压 3.3V */

/* 打印调试信息 */
#define assert_param(expr)          ((void)0U)

/* 包含 HAL 类型定义 (HAL_StatusTypeDef, I2C_HandleTypeDef 等) */
#include "stm32l4xx_hal_def.h"

/* ---- 按使能的模块包含对应头文件 ---- */
#if defined(HAL_CORTEX_MODULE_ENABLED)
#include "stm32l4xx_hal_cortex.h"
#endif
#if defined(HAL_DMA_MODULE_ENABLED)
#include "stm32l4xx_hal_dma.h"
#endif
#if defined(HAL_EXTI_MODULE_ENABLED)
#include "stm32l4xx_hal_exti.h"
#endif
#if defined(HAL_FLASH_MODULE_ENABLED)
#include "stm32l4xx_hal_flash.h"
#endif
#if defined(HAL_GPIO_MODULE_ENABLED)
#include "stm32l4xx_hal_gpio.h"
#endif
#if defined(HAL_I2C_MODULE_ENABLED)
#include "stm32l4xx_hal_i2c.h"
#endif
#if defined(HAL_PWR_MODULE_ENABLED)
#include "stm32l4xx_hal_pwr.h"
#endif
#if defined(HAL_RCC_MODULE_ENABLED)
#include "stm32l4xx_hal_rcc.h"
#endif
#if defined(HAL_RTC_MODULE_ENABLED)
#include "stm32l4xx_hal_rtc.h"
#endif
#if defined(HAL_SPI_MODULE_ENABLED)
#include "stm32l4xx_hal_spi.h"
#endif
#if defined(HAL_TIM_MODULE_ENABLED)
#include "stm32l4xx_hal_tim.h"
#endif
#if defined(HAL_UART_MODULE_ENABLED)
#include "stm32l4xx_hal_uart.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32L4xx_HAL_CONF_H */

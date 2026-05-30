/**
 * @file    system_stm32l4xx.c
 * @brief   CMSIS Cortex-M4 设备系统源文件 (STM32L4xx)
 *
 * 提供系统初始化、SystemCoreClock 变量更新
 * 注意：此文件由 STM32CubeMX 自动生成时可能不同，请以 CubeMX 版本为准
 */

#include "stm32l4xx.h"

/* 系统核心时钟频率 [Hz] */
uint32_t SystemCoreClock = 80000000U;

const uint8_t  AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t  APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};
const uint32_t MSIRangeTable[12] = {100000U, 200000U, 400000U, 800000U, 1000000U, 2000000U,
                                     4000000U, 8000000U, 16000000U, 24000000U, 32000000U, 48000000U};

/**
 * @brief  系统时钟配置前的初始化
 */
void SystemInit(void)
{
#if defined(USER_VECT_TAB_ADDRESS)
    SCB->VTOR = USER_VECT_TAB_ADDRESS;
#endif

    /* FPU 设置 */
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 20) | (3UL << 22));
#endif

    /* 配置 Flash 等待周期（由 HAL 在时钟配置中处理） */
    SystemCoreClock = 80000000U;
}

/**
 * @brief  更新 SystemCoreClock 变量
 */
void SystemCoreClockUpdate(void)
{
    uint32_t tmp, pllvco, pllr, pllm, plln;
    uint32_t pllsource;

    tmp = RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC;
    pllsource = (tmp == RCC_PLLCFGR_PLLSRC_HSE) ? HSE_VALUE : HSI_VALUE;

    pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1;
    plln = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
    pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1) * 2;

    pllvco = pllsource / pllm * plln;
    SystemCoreClock = pllvco / pllr;
}

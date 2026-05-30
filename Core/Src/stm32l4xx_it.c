/**
 * @file    stm32l4xx_it.c
 * @brief   中断服务函数实现
 */

#include "main.h"
#include "stm32l4xx_it.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* 外部引用 FreeRTOS 相关 */
extern void xPortSysTickHandler(void);

/******************************************************************************/
/*                          Cortex-M4 处理器异常                              */
/******************************************************************************/

void NMI_Handler(void) { while (1); }
void HardFault_Handler(void) { while (1); }
void MemManage_Handler(void) { while (1); }
void BusFault_Handler(void) { while (1); }
void UsageFault_Handler(void) { while (1); }

/* FreeRTOS 使用 PendSV / SysTick 进行任务切换 */
void SVC_Handler(void)        { /* FreeRTOS 内部处理 */ }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { /* FreeRTOS 内部处理 */ }
void SysTick_Handler(void)
{
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }
}

/******************************************************************************/
/*                          外设中断服务函数                                   */
/******************************************************************************/

void I2C1_EV_IRQHandler(void)  { HAL_I2C_EV_IRQHandler(&hi2c1); }
void I2C1_ER_IRQHandler(void)  { HAL_I2C_ER_IRQHandler(&hi2c1); }
void I2C2_EV_IRQHandler(void)  { HAL_I2C_EV_IRQHandler(&hi2c2); }
void I2C2_ER_IRQHandler(void)  { HAL_I2C_ER_IRQHandler(&hi2c2); }
void SPI1_IRQHandler(void)     { HAL_SPI_IRQHandler(&hspi1); }
void USART1_IRQHandler(void)   { HAL_UART_IRQHandler(&huart1); }
void USART3_IRQHandler(void)   { HAL_UART_IRQHandler(&huart3); }
void TIM2_IRQHandler(void)     { HAL_TIM_IRQHandler(&htim2); }

void EXTI0_IRQHandler(void)    { HAL_GPIO_EXTI_IRQHandler(BTN_MODE_PIN); }
void EXTI1_IRQHandler(void)    { HAL_GPIO_EXTI_IRQHandler(BTN_SELECT_PIN); }

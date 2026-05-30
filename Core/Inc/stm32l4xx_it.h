/**
 * @file    stm32l4xx_it.h
 * @brief   中断服务函数头文件
 */

#ifndef __STM32L4xx_IT_H
#define __STM32L4xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* 外设中断 */
void I2C1_EV_IRQHandler(void);
void I2C1_ER_IRQHandler(void);
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);
void SPI1_IRQHandler(void);
void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void TIM2_IRQHandler(void);
void EXTI0_IRQHandler(void);
void EXTI1_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32L4xx_IT_H */

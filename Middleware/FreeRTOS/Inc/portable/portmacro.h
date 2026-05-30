/**
 * @file    portmacro.h
 * @brief   FreeRTOS 移植层 — ARM Cortex-M4F (ARMCC v5 / RVDS)
 *
 * 适配: STM32L431RCT6 (Cortex-M4F, FPv4-SP, NVIC 4-bit优先级)
 * 编译器: ARMCC v5 (Keil MDK)
 */

#ifndef __PORTMACRO_H
#define __PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================== 类型定义 ========================== */
#define portCHAR          char
#define portFLOAT         float
#define portDOUBLE        double
#define portLONG          long
#define portSHORT         short
#define portSTACK_TYPE    uint32_t
#define portBASE_TYPE     long

typedef portSTACK_TYPE StackType_t;
typedef long           BaseType_t;
typedef unsigned long  UBaseType_t;

#if (configUSE_16_BIT_TICKS == 1)
    typedef uint16_t TickType_t;
    #define portMAX_DELAY (TickType_t)0xFFFFU
#else
    typedef uint32_t TickType_t;
    #define portMAX_DELAY (TickType_t)0xFFFFFFFFU
#endif

/* ========================== 架构常量 ========================== */
#define portBYTE_ALIGNMENT          8
#define portSTACK_GROWTH            (-1)
#define portTICK_PERIOD_MS          ((TickType_t)(1000U / configTICK_RATE_HZ))
#define portNOP()                   __nop()

/* ========================== NVIC 寄存器地址 ========================== */
#define portNVIC_INT_CTRL_REG       (*(volatile uint32_t *)0xE000ED04UL)
#define portNVIC_PENDSVSET_BIT      (1UL << 28UL)
#define portNVIC_SYSTICK_CTRL_REG   (*(volatile uint32_t *)0xE000E010UL)
#define portNVIC_SYSTICK_LOAD_REG   (*(volatile uint32_t *)0xE000E014UL)
#define portNVIC_SYSTICK_CURRENT    (*(volatile uint32_t *)0xE000E018UL)

#define portCPUID                   (*(volatile uint32_t *)0xE000ED00UL)

/* ========================== 任务调度 ========================== */
/* SVC #0 触发 PendSV — ARMCC v5 内联汇编语法 */
#define portYIELD()                 __asm { SVC 0 }

#define portYIELD_FROM_ISR(x)       portEND_SWITCHING_ISR(x)
#define portEND_SWITCHING_ISR(x)    do { if (x) { portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; } } while (0)

/* ========================== 临界区保护 ========================== */
/* ARMCC v5 使用 CMSIS intrinsic 函数，避免 GCC 风格内联汇编兼容性问题 */
#define portDISABLE_INTERRUPTS()    __disable_irq()
#define portENABLE_INTERRUPTS()     __enable_irq()

#define portENTER_CRITICAL()        vPortEnterCritical()
#define portEXIT_CRITICAL()         vPortExitCritical()

#define portSET_INTERRUPT_MASK_FROM_ISR()         ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)      vPortSetBASEPRI(x)

/* ========================== FPU 设置 ========================== */
#define portINITIAL_XPSR            (0x01000000UL)
#define portINITIAL_EXEC_RETURN     (0xFFFFFFFDU)

/* ========================== 任务函数声明宏 ========================== */
#define portTASK_FUNCTION_PROTO(vFunction, pvParameters)    void vFunction(void *pvParameters)
#define portTASK_FUNCTION(vFunction, pvParameters)          void vFunction(void *pvParameters)

/* ========================== MPU 相关 (本项目不使用) ========================== */
#define portPRIVILEGE_BIT           (0x80000000UL)
#define portUSING_MPU_WRAPPERS      0

/* ========================== 外部声明 ========================== */
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);
extern uint32_t ulPortRaiseBASEPRI(void);
extern void vPortSetBASEPRI(uint32_t ulNewMaskValue);

extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void vPortSVCHandler(void);

StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters);

/* ========================== 可选: 关中断计数 ========================== */
#if (configUSE_PORT_OPTIMISED_TASK_SELECTION == 1)
    #error "Optimised task selection not supported in this port"
#endif

#ifdef __cplusplus
}
#endif

#endif /* __PORTMACRO_H */

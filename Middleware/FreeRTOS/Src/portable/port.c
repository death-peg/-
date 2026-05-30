/**
 * @file    port.c
 * @brief   FreeRTOS 移植层 — ARM Cortex-M4F (ARMCC v5 / RVDS)
 *
 * 该文件实现任务上下文切换的底层代码。
 * 基于 FreeRTOS Kernel V10.x ARM_CM4F 移植，适配 ARMCC v5。
 *
 * 关键函数:
 *   xPortStartScheduler()   — 启动第一个任务
 *   vPortSVCHandler()       — SVC 异常 (启动任务切换)
 *   xPortPendSVHandler()    — PendSV 异常 (上下文切换)
 *   xPortSysTickHandler()   — SysTick 异常 (系统节拍)
 */

/* ----------------------- 标准 FreeRTOS 头文件 ----------------------- */
#include "FreeRTOS.h"
#include "task.h"

/* 如果启用了 MPU，需要额外包含 */
#if (configENABLE_MPU == 1)
    #include "mpu_wrappers.h"
#endif

/* ========================== 常量 ========================== */
#define portNVIC_SYSPRI2_REG       (*(volatile uint32_t *)0xE000ED20UL)
#define portNVIC_PENDSV_PRI        (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI       (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)

/* ========================== 前向声明 ========================== */
static void prvPortStartFirstTask(void);
static void prvEnableVFP(void);
extern void prvTaskExitError(void);   /* 任务退出错误处理, 在文件末尾定义 */

/* ========================== 全局变量 ========================== */

/**
 * @brief 每个任务维护自己的临界区嵌套计数。
 *        在上下文切换时保存/恢复。
 */
#if (configENABLE_MPU == 0)
    /* 单核非 MPU 模式下使用全局变量即可 */
    static volatile uint32_t ulCriticalNesting = 0xAAAAAAAAUL;
#else
    /* MPU 模式下每个任务需要独立计数，由 pxPortInitialiseStack() 初始化 */
    volatile uint32_t ulCriticalNesting = 0xAAAAAAAAUL;
#endif

/* ========================== 栈初始化 ========================== */

/**
 * @brief  初始化任务栈帧，模拟异常入栈后的寄存器布局。
 *
 * Cortex-M4 异常入栈顺序 (从高地址到低地址):
 *   xPSR, PC, LR, R12, R3, R2, R1, R0
 * 然后软压入:
 *   R11, R10, R9, R8, R7, R6, R5, R4
 * 最后:
 *   EXC_RETURN (用于 FPU 上下文)
 *
 * @param pxTopOfStack  栈顶指针 (高地址端)
 * @param pxCode        任务函数入口
 * @param pvParameters   任务参数
 * @return              调整后的栈顶指针
 */
StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                    TaskFunction_t pxCode,
                                    void *pvParameters)
{
    /* 异常返回时硬件自动出栈 (从低地址开始):
     * 预留 R0, R1, R2, R3, R12, LR, PC, xPSR 共 8 个字
     */
    pxTopOfStack--;
    *pxTopOfStack = portINITIAL_XPSR;           /* xPSR — Thumb 状态           */
    pxTopOfStack--;
    *pxTopOfStack = ((StackType_t)pxCode) & 0xFFFFFFFEUL; /* PC (bit0=1 Thumb) */
    pxTopOfStack--;
    *pxTopOfStack = (StackType_t)prvTaskExitError;       /* LR — 任务返回地址    */
    pxTopOfStack -= 5;                                   /* R12, R3, R2, R1      */
    *pxTopOfStack = (StackType_t)pvParameters;            /* R0 — 任务参数        */

    /* 软件保存的寄存器: R11..R4 */
    pxTopOfStack -= 8;
    /* R11..R4 初始化为已知值 (可选, 此处留0) */

    /* EXC_RETURN: 使用 FPU 扩展帧 */
    pxTopOfStack--;
    *pxTopOfStack = 0xFFFFFFFDU; /* EXC_RETURN with FPU extended frame */

    return pxTopOfStack;
}

/* ========================== 启动调度器 ========================== */

/**
 * @brief  FreeRTOS 调度器启动入口。
 *         设置 PendSV / SysTick 优先级为最低，然后触发 SVC 启动第一个任务。
 */
BaseType_t xPortStartScheduler(void)
{
    /* 使能 FPU (CP10, CP11) */
    prvEnableVFP();

    /* 设置 PendSV 为最低中断优先级 */
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;

    /* 设置 SysTick 为最低中断优先级 */
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

    /* 初始化 SysTick: 禁用, 计数值清零 */
    portNVIC_SYSTICK_CTRL_REG  = 0UL;
    portNVIC_SYSTICK_CURRENT   = 0UL;
    portNVIC_SYSTICK_LOAD_REG  = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;

    /* 设置 SysTick 时钟源为 Core Clock, 使能中断, 启动计数 */
    portNVIC_SYSTICK_CTRL_REG  = 0x00000007UL; /* CLKSOURCE | TICKINT | ENABLE */

    /* 启动第一个任务 (SVC 异常) */
    prvPortStartFirstTask();

    /* 不应返回 */
    return 0;
}

/* ========================== 启动第一个任务 ========================== */

/**
 * @brief  通过 SVC 异常启动第一个任务。
 *         ARMCC v5 使用 __asm 嵌入式汇编函数。
 */
__asm static void prvPortStartFirstTask(void)
{
    PRESERVE8

    ldr  r0, =0xE000ED08        /* VTOR 寄存器地址 */
    ldr  r0, [r0]               /* 读取向量表基址    */
    ldr  r0, [r0]               /* 读取 MSP 初始值   */
    msr  msp, r0                /* 设置 MSP          */
    cpsie i                     /* 全局使能中断      */
    cpsie f
    dsb
    isb
    svc  0                      /* 触发 SVC 异常     */
    nop
    bx   lr
    ALIGN
}

/* ========================== SVC 异常处理 ========================== */

/**
 * @brief  SVC 异常处理 — 用于启动第一个任务。
 *         从任务栈中恢复上下文，执行异常返回以进入第一个任务。
 */
__asm void vPortSVCHandler(void)
{
    PRESERVE8
    IMPORT  pxCurrentTCB

    ldr  r3, =pxCurrentTCB      /* 获取当前 TCB 指针   */
    ldr  r1, [r3]               /* r1 = pxCurrentTCB   */
    ldr  r0, [r1]               /* r0 = 栈顶指针        */
    ldmia r0!, {r4-r11}         /* 恢复 R4-R11         */
    msr  psp, r0                /* 设置 PSP            */
    isb
    mov  r0, #0
    msr  basepri, r0            /* 清除 BASEPRI        */
    orr  r14, #0xd              /* EXC_RETURN → PSP + FPU */
    ALIGN
    bx   r14                    /* 异常返回, 进入任务   */
}

/* ========================== PendSV 异常处理 (上下文切换) ========================== */

/**
 * @brief  PendSV 异常处理 — 执行任务上下文切换。
 *         保存当前任务上下文 → 选择下一个任务 → 恢复新任务上下文。
 *
 * 寄存器保存/恢复顺序:
 *   保存: R4-R11 + PSP → 当前任务栈
 *   恢复: 新任务栈 → R4-R11 + PSP
 */
__asm void xPortPendSVHandler(void)
{
    IMPORT  pxCurrentTCB
    IMPORT  vTaskSwitchContext
    PRESERVE8

    mrs  r0, psp                /* 读取 PSP (当前任务栈指针) */
    isb

    ldr  r3, =pxCurrentTCB      /* 获取当前 TCB 指针地址  */
    ldr  r2, [r3]               /* r2 = pxCurrentTCB      */

    stmdb r0!, {r4-r11}         /* 保存 R4-R11 到任务栈   */
    str  r0, [r2]               /* 更新 TCB 中的栈顶指针   */

    stmdb sp!, {r3, r14}        /* 保存 R3 和 LR 到系统栈  */
    mov  r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY  /* 屏蔽低优先级中断 */
    msr  basepri, r0
    dsb
    isb
    bl   vTaskSwitchContext     /* 选择下一个任务          */
    mov  r0, #0
    msr  basepri, r0            /* 恢复中断                */
    ldmia sp!, {r3, r14}        /* 恢复 R3 和 LR           */

    ldr  r1, [r3]               /* r1 = 新的 pxCurrentTCB */
    ldr  r0, [r1]               /* r0 = 新任务栈顶指针     */
    ldmia r0!, {r4-r11}         /* 恢复 R4-R11            */
    msr  psp, r0                /* 更新 PSP               */
    isb
    bx   r14                    /* 异常返回                */
    ALIGN
}

/* ========================== SysTick 处理 ========================== */

/**
 * @brief  SysTick 异常处理 — FreeRTOS 系统节拍。
 *         调用 xTaskIncrementTick() 进行延时/时间片判断。
 */
void xPortSysTickHandler(void)
{
    /* 屏蔽低优先级中断 */
    portSET_INTERRUPT_MASK_FROM_ISR();
    {
        if (xTaskIncrementTick() != pdFALSE)
        {
            /* 需要上下文切换 */
            portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR(0);
}

/* ========================== 临界区管理 ========================== */

/**
 * @brief  进入临界区 — 关中断并增加嵌套计数。
 */
void vPortEnterCritical(void)
{
    portDISABLE_INTERRUPTS();
    ulCriticalNesting++;

    /* 首次进入临界区不需要额外操作 */
    if (ulCriticalNesting == 1)
    {
        /* 可选的调试钩子 */
        (void)0;
    }
}

/**
 * @brief  退出临界区 — 递减嵌套计数，嵌套为 0 时开中断。
 */
void vPortExitCritical(void)
{
    if (ulCriticalNesting > 0)
    {
        ulCriticalNesting--;
    }

    if (ulCriticalNesting == 0)
    {
        portENABLE_INTERRUPTS();
    }
}

/* ========================== BASEPRI 管理 (中断安全版本) ========================== */

/**
 * @brief  提升 BASEPRI 以屏蔽低优先级中断，返回原 BASEPRI 值。
 *         可在 ISR 中安全调用。ARMCC v5 使用 __asm 嵌入式汇编函数。
 */
__asm uint32_t ulPortRaiseBASEPRI(void)
{
    PRESERVE8

    mrs  r0, basepri
    mov  r1, #configMAX_SYSCALL_INTERRUPT_PRIORITY
    msr  basepri, r1
    dsb
    isb
    bx   lr
    ALIGN
}

/**
 * @brief  恢复 BASEPRI 到之前的值。
 *         参数通过 R0 传递 (ARMCC v5 嵌入式汇编约定)。
 */
__asm void vPortSetBASEPRI(uint32_t ulNewMaskValue)
{
    PRESERVE8

    msr  basepri, r0
    dsb
    isb
    bx   lr
    ALIGN
}

/* ========================== FPU 使能 ========================== */

/**
 * @brief  使能 FPU (浮点运算单元)。
 *         设置 CPACR 中 CP10 和 CP11 为完全访问权限。
 *         使用 C 直接操作寄存器，避免 ARMCC v5 __asm{} 内联汇编限制。
 */
__forceinline static void prvEnableVFP(void)
{
    /* SCB->CPACR |= (0xF << 20) — CP10 & CP11 full access */
    volatile uint32_t * const cpacr = (uint32_t *)0xE000ED88UL;
    *cpacr |= (0xFUL << 20);
    __dsb(0xF);
    __isb(0xF);
}

/* ========================== 任务退出错误处理 ========================== */

/**
 * @brief  任务函数不应返回。如果返回，捕获错误。
 */
void prvTaskExitError(void)
{
    /* 任务函数不应返回。此处进入死循环。 */
    __disable_irq();
    for (;;)
    {
        /* 可在调试器中设置断点于此 */
    }
}

/* ========================== 断言配置 ========================== */

/**
 * @brief  FreeRTOS 断言失败时调用。
 *         禁用中断并死循环，便于调试器捕获。
 */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t     *pulIdleTaskStackSize)
{
    /* 如果使用静态分配，需要在此提供内存。
     * 当前使用动态分配 (configSUPPORT_STATIC_ALLOCATION = 0)，
     * 因此此函数不会被调用，但保留以避免链接错误。 */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t  **ppxTimerTaskStackBuffer,
                                     uint32_t     *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t  uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}

/* ========================== 调度器停止 (占位) ========================== */

/**
 * @brief  停止 FreeRTOS 调度器 (本项目不使用，提供空实现满足链接)。
 */
void vPortEndScheduler(void)
{
    /* 本项目不实现调度器停止功能 */
    __disable_irq();
    for (;;);
}

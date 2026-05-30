/**
 * @file    STM32L431RCTx.ld
 * @brief   GCC 链接脚本 — STM32L431RCTx
 *
 * 内存布局:
 *   Flash: 256KB @ 0x08000000
 *   SRAM1:  64KB @ 0x20000000
 *
 * 段布局 (Flash):
 *   .isr_vector     异常向量表
 *   .text           代码 + 只读常量
 *   .rodata         只读数据
 *   .ARM.extab / .ARM.exidx / .preinit_array / .init_array / .fini_array
 *
 * 段布局 (RAM):
 *   .data           已初始化全局变量 (从 Flash 复制)
 *   .bss            未初始化全局变量 (启动时清零)
 *   ._user_heap_stack  堆 + 栈 (栈顶 = _estack)
 */

MEMORY
{
    FLASH  (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM    (rwx) : ORIGIN = 0x20000000, LENGTH =  64K
}

/* 栈顶 = RAM 末尾 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* 最小堆大小 (FreeRTOS 使用自己的堆，此处留 0) */
_Min_Heap_Size  = 0x000;
/* 栈大小 (系统栈 + 中断栈) */
_Min_Stack_Size = 0x800;

SECTIONS
{
    /* ==================== Flash 段 ==================== */

    .isr_vector :
    {
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    .ARM.extab :
    {
        . = ALIGN(4);
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
        . = ALIGN(4);
    } > FLASH

    .ARM.exidx :
    {
        . = ALIGN(4);
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
        . = ALIGN(4);
    } > FLASH

    .preinit_array :
    {
        PROVIDE(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE(__fini_array_end = .);
    } > FLASH

    /* 初始化数据加载地址 (Flash 中) */
    _sidata = LOADADDR(.data);

    /* ==================== RAM 段 ==================== */

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* ==================== 堆 + 栈 ==================== */

    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /* 移除调试段以减小体积 (--gc-sections 会自动清理未使用的函数/数据) */
    /DISCARD/ :
    {
        *(.ARM.attributes)
    }
}

# ============================================================
# SmartBand — 智能健康监测手环
# STM32L431RCTx + FreeRTOS + HAL
#
# 构建方法:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
#   cmake --build build
#
# 生成产物: build/SmartBand.elf / .hex / .bin
# ============================================================

cmake_minimum_required(VERSION 3.20)
project(SmartBand C ASM)

# ============================================================
# 1. MCU 编译选项
# ============================================================

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfloat-abi=hard
    -mfpu=fpv4-sp-d16
)

set(CMAKE_C_FLAGS_INIT           "${CPU_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_FLAGS_DEBUG          "-Og -g3")
set(CMAKE_C_FLAGS_RELEASE        "-Os -g0 -DNDEBUG")
set(CMAKE_ASM_FLAGS_INIT         "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT  "${CPU_FLAGS} -Wl,--gc-sections -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map -specs=nosys.specs -specs=nano.specs -u _printf_float")

# 链接脚本
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/STM32L431RCTx.ld")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -T${LINKER_SCRIPT}")

# ============================================================
# 2. 全局宏定义
# ============================================================

add_compile_definitions(
    USE_HAL_DRIVER
    STM32L431xx
    HSE_VALUE=12000000
    HSI_VALUE=16000000
    __FPU_PRESENT=1
    __FPU_USED=1
)

# ============================================================
# 3. 收集源文件
# ============================================================

# --- 启动文件 (GCC 汇编) ---
set(STARTUP_SRC
    Drivers/CMSIS/Device/ST/STM32L4xx/Source/Templates/gcc/startup_stm32l431xx.s
)

# --- Core ---
set(CORE_SRC
    Core/Src/main.c
    Core/Src/stm32l4xx_it.c
    Core/Src/system_stm32l4xx.c
)

# --- App (FreeRTOS Task 层) ---
file(GLOB APP_SRC App/Src/*.c)

# --- 硬件驱动 ---
file(GLOB HW_DRV_SRC HW_Drivers/Src/*.c)

# --- HAL 驱动 (按需裁剪) ---
set(HAL_SRC
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_exti.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rtc.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rtc_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_spi.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_spi_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c
)

# --- FreeRTOS 内核 ---
set(FREERTOS_KERNEL_SRC
    Middleware/FreeRTOS/Src/tasks.c
    Middleware/FreeRTOS/Src/queue.c
    Middleware/FreeRTOS/Src/list.c
    Middleware/FreeRTOS/Src/timers.c
    Middleware/FreeRTOS/Src/event_groups.c
    Middleware/FreeRTOS/Src/stream_buffer.c
    Middleware/FreeRTOS/Src/croutine.c
    Middleware/FreeRTOS/Src/heap_4.c
)

# --- FreeRTOS 移植层 ---
set(FREERTOS_PORT_SRC
    Middleware/FreeRTOS/Src/portable/port.c
)

# --- CMSIS-RTOS2 适配 ---
set(FREERTOS_CMSIS_SRC
    Middleware/FreeRTOS/Src/cmsis_os2.c
)

# --- 算法 ---
file(GLOB ALGO_SRC  Middleware/Algorithm/Src/*.c)

# --- 协议 ---
file(GLOB PROTO_SRC Middleware/Protocol/Src/*.c)

# ============================================================
# 4. 汇总所有源文件
# ============================================================

set(ALL_SOURCES
    ${STARTUP_SRC}
    ${CORE_SRC}
    ${APP_SRC}
    ${HW_DRV_SRC}
    ${HAL_SRC}
    ${FREERTOS_KERNEL_SRC}
    ${FREERTOS_PORT_SRC}
    ${FREERTOS_CMSIS_SRC}
    ${ALGO_SRC}
    ${PROTO_SRC}
)

# ============================================================
# 5. 创建可执行目标
# ============================================================

add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# ============================================================
# 6. 头文件搜索路径
# ============================================================

target_include_directories(${PROJECT_NAME} PRIVATE
    Core/Inc
    App/Inc
    HW_Drivers/Inc
    Middleware/FreeRTOS/Inc
    Middleware/FreeRTOS/Inc/portable
    Middleware/Algorithm/Inc
    Middleware/Protocol/Inc
    Drivers/CMSIS/Include
    Drivers/CMSIS/Core/Include
    Drivers/CMSIS/Device/ST/STM32L4xx/Include
    Drivers/CMSIS/RTOS2/Include
    Drivers/STM32L4xx_HAL_Driver/Inc
)

# ============================================================
# 7. 链接库
# ============================================================

target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,--start-group
    -lc -lm -lgcc -lnosys
    -Wl,--end-group
)

# ============================================================
# 8. 编译选项 (每个文件)
# ============================================================

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:   ${CPU_FLAGS} -ffunction-sections -fdata-sections -fno-common>
    $<$<COMPILE_LANGUAGE:ASM>: ${CPU_FLAGS} -x assembler-with-cpp>
)

# ============================================================
# 9. 生成后处理 — .hex / .bin / size
# ============================================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Generating .hex, .bin and size report..."
)

# ============================================================
# 10. 打印构建配置摘要
# ============================================================

message(STATUS "============================================")
message(STATUS "  SmartBand CMake Build")
message(STATUS "  MCU:   STM32L431RCTx (Cortex-M4F)")
message(STATUS "  Flash: 256KB | RAM: 64KB")
message(STATUS "  HSE:   12MHz | SysClk: 80MHz")
message(STATUS "  RTOS:  FreeRTOS (CMSIS-RTOS2 API)")
message(STATUS "  Build: ${CMAKE_BUILD_TYPE}")
message(STATUS "============================================")

# ============================================================
# CMake 工具链文件 — ARM Cortex-M4 (arm-none-eabi-gcc)
# 适用于 STM32L431RCTx
#
# 使用方法:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-gcc.cmake
#   cmake --build build
# ============================================================

set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)

# --- 编译器前缀 (自动检测 D:\ARMgnu, 否则依赖 PATH) ---
if(NOT DEFINED TOOLCHAIN_ROOT)
    foreach(_path "D:/ARMgnu" "C:/Program Files (x86)/GNU Arm Embedded Toolchain" "C:/Program Files (x86)/GNU Tools ARM Embedded")
        # zip 解压后通常一层子目录, msi 安装后可能直接是 bin/
        file(GLOB _found1 "${_path}/*/bin/arm-none-eabi-gcc.exe")
        file(GLOB _found2 "${_path}/bin/arm-none-eabi-gcc.exe")
        if(_found1)
            get_filename_component(_bin_dir "${_found1}" DIRECTORY)
            set(TOOLCHAIN_ROOT "${_bin_dir}/")
            break()
        elseif(_found2)
            get_filename_component(_bin_dir "${_found2}" DIRECTORY)
            set(TOOLCHAIN_ROOT "${_bin_dir}/")
            break()
        endif()
    endforeach()
endif()

if(DEFINED TOOLCHAIN_ROOT)
    message(STATUS "ARM GCC found: ${TOOLCHAIN_ROOT}")
    set(CMAKE_C_COMPILER        "${TOOLCHAIN_ROOT}arm-none-eabi-gcc.exe")
    set(CMAKE_CXX_COMPILER      "${TOOLCHAIN_ROOT}arm-none-eabi-g++.exe")
    set(CMAKE_ASM_COMPILER      "${TOOLCHAIN_ROOT}arm-none-eabi-gcc.exe")
    set(CMAKE_AR                "${TOOLCHAIN_ROOT}arm-none-eabi-gcc-ar.exe")
    set(CMAKE_OBJCOPY           "${TOOLCHAIN_ROOT}arm-none-eabi-objcopy.exe")
    set(CMAKE_OBJDUMP           "${TOOLCHAIN_ROOT}arm-none-eabi-objdump.exe")
    set(CMAKE_SIZE              "${TOOLCHAIN_ROOT}arm-none-eabi-size.exe")
else()
    message(STATUS "ARM GCC: using PATH")
    set(CMAKE_C_COMPILER        arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER      arm-none-eabi-g++)
    set(CMAKE_ASM_COMPILER      arm-none-eabi-gcc)
    set(CMAKE_AR                arm-none-eabi-gcc-ar)
    set(CMAKE_OBJCOPY           arm-none-eabi-objcopy)
    set(CMAKE_OBJDUMP           arm-none-eabi-objdump)
    set(CMAKE_SIZE              arm-none-eabi-size)
endif()

# 使用静态库类型避免 CMake 尝试运行交叉编译的测试程序
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 搜索规则: 只在目标 sysroot 中查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

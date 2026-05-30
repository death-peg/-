/**
 * @file    main.c
 * @brief   智能手环 - 主程序入口
 *
 * 功能概述：
 *   1. 心率监测 (MAX30102 PPG 传感器, 100Hz)
 *   2. 血压估算 (基于 PTT / 血管特征参数, 1Hz)
 *   3. 撞击检测 (ADXL345 加速度计, 200Hz)
 *   4. 秒表计时 (TIM2 硬件定时 + 任务刷新)
 *   5. 超限无线报警 (nRF24L01+ 发送至接收端)
 *   6. OLED 实时数据显示
 *   7. 振动马达触觉反馈
 *
 * 任务优先级：撞击 > 心率 > 报警 > 计时 > 血压 > 显示 > 系统监控
 */

/* ========================== 头文件 ========================== */
#include "main.h"
#include "cmsis_os2.h"
#include "app_manager.h"

/* ========================== 全局外设句柄 ========================== */
I2C_HandleTypeDef   hi2c1;          /* MAX30102 心率传感器  */
I2C_HandleTypeDef   hi2c2;          /* SSD1306 OLED 显示   */
SPI_HandleTypeDef   hspi1;          /* ADXL345 加速度计     */
SPI_HandleTypeDef   hspi2;          /* nRF24L01+ 无线收发   */
UART_HandleTypeDef  huart1;         /* 调试串口             */
UART_HandleTypeDef  huart3;         /* Air780E 4G 模块      */
TIM_HandleTypeDef   htim2;          /* 秒表计时基准         */

/* ========================== 系统时钟配置 ========================== */
/**
 * @brief  配置系统时钟为 80MHz
 *         HSE(12MHz) -> PLLM(/3) -> PLLN(*40) -> PLLR(/2) = 80MHz
 *         APB1 = 80MHz, APB2 = 80MHz
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* ---- 配置 HSE 振荡器 ---- */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 3;                    /* 12MHz / 3 = 4MHz  */
    RCC_OscInitStruct.PLL.PLLN       = 40;                   /* 4MHz * 40 = 160MHz */
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ       = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR       = RCC_PLLR_DIV2;        /* 160MHz / 2 = 80MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    /* ---- 配置系统时钟分频 ---- */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;       /* HCLK = 80MHz   */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;         /* APB1 = 80MHz   */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;         /* APB2 = 80MHz   */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();

    /* ---- 配置外设时钟源 ---- */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
        Error_Handler();

    /* 使能 FPU (浮点运算单元) */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    SCB->CPACR |= ((3UL << 20) | (3UL << 22));  /* CP10, CP11 full access */
}

/* ========================== 外设初始化 (占位，CubeMX 生成后替换) ========================== */

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* 按键: PA0(Mode), PA1(Select) - 上拉输入 */
    GPIO_InitStruct.Pin  = BTN_MODE_PIN | BTN_SELECT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 振动马达: PA2 - 推挽输出 */
    GPIO_InitStruct.Pin  = VIBRATOR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* MAX30102 INT: PB1 - 输入 */
    GPIO_InitStruct.Pin  = MAX30102_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.Timing          = 0x10909CEC;      /* 400kHz @80MHz */
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

void MX_I2C2_Init(void)
{
    hi2c2.Instance             = I2C2;
    hi2c2.Init.Timing          = 0x10909CEC;      /* 400kHz */
    hi2c2.Init.OwnAddress1     = 0;
    hi2c2.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c2);
}

void MX_SPI1_Init(void)
{
    hspi1.Instance            = SPI1;
    hspi1.Init.Mode           = SPI_MODE_MASTER;
    hspi1.Init.Direction      = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize       = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity    = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase       = SPI_PHASE_2EDGE;
    hspi1.Init.NSS            = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  /* 5MHz */
    hspi1.Init.FirstBit       = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode         = SPI_TIMODE_DISABLE;
    HAL_SPI_Init(&hspi1);
}

void MX_SPI2_Init(void)
{
    hspi2.Instance            = SPI2;
    hspi2.Init.Mode           = SPI_MODE_MASTER;
    hspi2.Init.Direction      = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize       = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity    = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase       = SPI_PHASE_1EDGE;
    hspi2.Init.NSS            = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;   /* 10MHz */
    hspi2.Init.FirstBit       = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode         = SPI_TIMODE_DISABLE;
    HAL_SPI_Init(&hspi2);
}

void MX_USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

void MX_USART3_UART_Init(void)
{
    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 115200;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart3);
}

void Air780E_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PWRKEY: PC13 - 推挽输出, 默认高 */
    GPIO_InitStruct.Pin   = AIR780E_PWRKEY_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(AIR780E_PWRKEY_PORT, AIR780E_PWRKEY_PIN, GPIO_PIN_SET);

    /* RESET: PC14 - 推挽输出, 默认高 */
    GPIO_InitStruct.Pin  = AIR780E_RESET_PIN;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    HAL_GPIO_WritePin(AIR780E_RESET_PORT, AIR780E_RESET_PIN, GPIO_PIN_SET);

    /* NET_STATUS: PC15 - 输入 */
    GPIO_InitStruct.Pin  = AIR780E_NETSTAT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void MX_TIM2_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim2.Instance         = TIM2;
    htim2.Init.Prescaler   = 7999;             /* 80MHz / 8000 = 10kHz */
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period      = 9999;             /* 10kHz / 10000 = 1s 溢出 */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* 错误死锁 - 可在此处加 LED 闪烁指示 */
    }
}

/* ========================== 主函数 ========================== */
int main(void)
{
    /* HAL 库初始化 */
    HAL_Init();

    /* 系统时钟配置 */
    SystemClock_Config();

    /* 外设初始化 */
    MX_GPIO_Init();
    Air780E_GPIO_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    MX_TIM2_Init();

    /* 启动硬件定时器 */
    HAL_TIM_Base_Start(&STOPWATCH_TIM);

    /* ---- 内核初始化与 FreeRTOS 任务创建 ---- */
    App_Manager_Init();

    /* 启动 FreeRTOS 调度器（永不返回） */
    osKernelStart();

    /* 不应到达此处 */
    while (1)
    {
        Error_Handler();
    }
}

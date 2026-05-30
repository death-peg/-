/**
 * @file    hw_oled_ssd1306.h
 * @brief   SSD1306 OLED 128x64 I2C 驱动 - 头文件
 *
 * 通信接口: I2C2 (400kHz)
 * 分辨率:   128 x 64
 */

#ifndef __HW_OLED_SSD1306_H
#define __HW_OLED_SSD1306_H

#include "main.h"
#include <stdint.h>

/* ========================== SSD1306 命令 ========================== */
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_DISPLAY_ALL_ON      0xA5
#define SSD1306_CMD_DISPLAY_NORMAL      0xA6
#define SSD1306_CMD_DISPLAY_INVERT      0xA7
#define SSD1306_CMD_SET_MUX_RATIO       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_SEG_REMAP           0xA1    /* 列地址重映射  */
#define SSD1306_CMD_COM_SCAN_DEC        0xC8    /* COM 反向扫描  */
#define SSD1306_CMD_COM_SCAN_INC        0xC0
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_DISPLAY_CLK     0xD5
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOM_DETECT     0xDB
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_SET_COL_ADDR        0x21
#define SSD1306_CMD_SET_PAGE_ADDR       0x22

/* ========================== 显示参数 ========================== */
#define SSD1306_WIDTH                   128
#define SSD1306_HEIGHT                  64
#define SSD1306_PAGES                   8        /* 64 / 8 = 8 pages */

/* ========================== API 函数声明 ========================== */

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_SetContrast(uint8_t contrast);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t font_size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t font_size);
void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);

/* ---- 底层 I2C ---- */
void OLED_WriteCmd(uint8_t cmd);
void OLED_WriteData(uint8_t data);

#endif /* __HW_OLED_SSD1306_H */

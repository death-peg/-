/**
 * @file    hw_oled_ssd1306.c
 * @brief   SSD1306 OLED 128x64 I2C 驱动实现
 */

#include "hw_oled_ssd1306.h"
#include <string.h>

/* ========================== 5x7 字体表 (ASCII 32-127) ========================== */
/* 此处仅放置字体查找逻辑，完整字体表通常在单独的 font.c 中 */
static const uint8_t font_5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* !     */
    {0x00,0x07,0x00,0x07,0x00}, /* "     */
    {0x14,0x7F,0x14,0x7F,0x14}, /* #     */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $     */
    {0x23,0x13,0x08,0x64,0x62}, /* %     */
    {0x36,0x49,0x55,0x22,0x50}, /* &     */
    {0x00,0x05,0x03,0x00,0x00}, /* '     */
    {0x00,0x1C,0x22,0x41,0x00}, /* (     */
    {0x00,0x41,0x22,0x1C,0x00}, /* )     */
    {0x08,0x2A,0x1C,0x2A,0x08}, /* *     */
    {0x08,0x08,0x3E,0x08,0x08}, /* +     */
    {0x00,0x50,0x30,0x00,0x00}, /* ,     */
    {0x08,0x08,0x08,0x08,0x08}, /* -     */
    {0x00,0x60,0x60,0x00,0x00}, /* .     */
    {0x20,0x10,0x08,0x04,0x02}, /* /     */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0     */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1     */
    {0x42,0x61,0x51,0x49,0x46}, /* 2     */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3     */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4     */
    {0x27,0x45,0x45,0x45,0x39}, /* 5     */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6     */
    {0x01,0x71,0x09,0x05,0x03}, /* 7     */
    {0x36,0x49,0x49,0x49,0x36}, /* 8     */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9     */
    {0x00,0x36,0x36,0x00,0x00}, /* :     */
    {0x00,0x56,0x36,0x00,0x00}, /* ;     */
    {0x00,0x08,0x14,0x22,0x41}, /* <     */
    {0x14,0x14,0x14,0x14,0x14}, /* =     */
    {0x41,0x22,0x14,0x08,0x00}, /* >     */
    {0x02,0x01,0x51,0x09,0x06}, /* ?     */
    {0x32,0x49,0x79,0x41,0x3E}, /* @     */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A     */
    {0x7F,0x49,0x49,0x49,0x36}, /* B     */
    {0x3E,0x41,0x41,0x41,0x22}, /* C     */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D     */
    {0x7F,0x49,0x49,0x49,0x41}, /* E     */
    {0x7F,0x09,0x09,0x01,0x01}, /* F     */
    {0x3E,0x41,0x41,0x51,0x32}, /* G     */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H     */
    {0x00,0x41,0x7F,0x41,0x00}, /* I     */
    {0x20,0x40,0x41,0x3F,0x01}, /* J     */
    {0x7F,0x08,0x14,0x22,0x41}, /* K     */
    {0x7F,0x40,0x40,0x40,0x40}, /* L     */
    {0x7F,0x02,0x04,0x02,0x7F}, /* M     */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N     */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O     */
    {0x7F,0x09,0x09,0x09,0x06}, /* P     */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q     */
    {0x7F,0x09,0x19,0x29,0x46}, /* R     */
    {0x46,0x49,0x49,0x49,0x31}, /* S     */
    {0x01,0x01,0x7F,0x01,0x01}, /* T     */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U     */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V     */
    {0x7F,0x20,0x18,0x20,0x7F}, /* W     */
    {0x63,0x14,0x08,0x14,0x63}, /* X     */
    {0x03,0x04,0x78,0x04,0x03}, /* Y     */
    {0x61,0x51,0x49,0x45,0x43}, /* Z     */
    {0x00,0x00,0x7F,0x41,0x41}, /* [     */
    {0x02,0x04,0x08,0x10,0x20}, /* \     */
    {0x41,0x41,0x7F,0x00,0x00}, /* ]     */
    {0x04,0x02,0x01,0x02,0x04}, /* ^     */
    {0x40,0x40,0x40,0x40,0x40}, /* _     */
    {0x00,0x01,0x02,0x04,0x00}, /* `     */
    /* 小写字母略，实际项目需补充完整 */
};

/* ========================== 显示缓冲区 ========================== */
static uint8_t OLED_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* ========================== 底层 I2C 操作 ========================== */

void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};   /* 控制字节 + 命令 */
    HAL_I2C_Master_Transmit(&OLED_I2C, OLED_I2C_ADDR, buf, 2, 100);
}

void OLED_WriteData(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};  /* 控制字节 + 数据 */
    HAL_I2C_Master_Transmit(&OLED_I2C, OLED_I2C_ADDR, buf, 2, 100);
}

/* ========================== 初始化 ========================== */

void OLED_Init(void)
{
    HAL_Delay(100);  /* 上电稳定 */

    OLED_WriteCmd(SSD1306_CMD_DISPLAY_OFF);

    OLED_WriteCmd(SSD1306_CMD_SET_DISPLAY_CLK);
    OLED_WriteCmd(0x80);    /* 推荐值 */

    OLED_WriteCmd(SSD1306_CMD_SET_MUX_RATIO);
    OLED_WriteCmd(0x3F);    /* 64 MUX */

    OLED_WriteCmd(SSD1306_CMD_SET_DISPLAY_OFFSET);
    OLED_WriteCmd(0x00);

    OLED_WriteCmd(SSD1306_CMD_SET_START_LINE);

    OLED_WriteCmd(SSD1306_CMD_CHARGE_PUMP);
    OLED_WriteCmd(0x14);    /* 使能电荷泵 (3.3V → 7.5V OLED) */

    OLED_WriteCmd(SSD1306_CMD_MEMORY_MODE);
    OLED_WriteCmd(0x00);    /* 水平寻址模式 */

    OLED_WriteCmd(SSD1306_CMD_SEG_REMAP);       /* 列 127 映射到 SEG0 */
    OLED_WriteCmd(SSD1306_CMD_COM_SCAN_DEC);    /* COM[N-1] 扫描到 COM0 */

    OLED_WriteCmd(SSD1306_CMD_SET_COM_PINS);
    OLED_WriteCmd(0x12);    /* Alternative COM pin config */

    OLED_WriteCmd(SSD1306_CMD_SET_CONTRAST);
    OLED_WriteCmd(0x7F);    /* 中等对比度 */

    OLED_WriteCmd(SSD1306_CMD_SET_PRECHARGE);
    OLED_WriteCmd(0x22);    /* Phase1=2 DCLK, Phase2=2 DCLK */

    OLED_WriteCmd(SSD1306_CMD_SET_VCOM_DETECT);
    OLED_WriteCmd(0x40);    /* ~0.77 x VCC */

    OLED_WriteCmd(SSD1306_CMD_DISPLAY_NORMAL);
    OLED_WriteCmd(SSD1306_CMD_DISPLAY_ON);

    OLED_Clear();
    OLED_Refresh();
}

/* ========================== 显示操作 ========================== */

void OLED_Clear(void)
{
    memset(OLED_Buffer, 0x00, sizeof(OLED_Buffer));
}

void OLED_Refresh(void)
{
    /* 设置页寻址范围 0-7 */
    OLED_WriteCmd(SSD1306_CMD_SET_PAGE_ADDR);
    OLED_WriteCmd(0);
    OLED_WriteCmd(7);

    /* 设置列寻址范围 0-127 */
    OLED_WriteCmd(SSD1306_CMD_SET_COL_ADDR);
    OLED_WriteCmd(0);
    OLED_WriteCmd(127);

    /* 批量写入显示数据 */
    for (uint16_t i = 0; i < sizeof(OLED_Buffer); i += 32)
    {
        uint8_t buf[33];
        buf[0] = 0x40;  /* 数据模式 */
        uint8_t len = (sizeof(OLED_Buffer) - i > 32) ? 32 : (uint8_t)(sizeof(OLED_Buffer) - i);
        memcpy(buf + 1, OLED_Buffer + i, len);
        HAL_I2C_Master_Transmit(&OLED_I2C, OLED_I2C_ADDR, buf, len + 1, 100);
    }
}

void OLED_SetContrast(uint8_t contrast)
{
    OLED_WriteCmd(SSD1306_CMD_SET_CONTRAST);
    OLED_WriteCmd(contrast);
}

void OLED_DrawPixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint16_t index = x + (y / 8) * SSD1306_WIDTH;

    if (color)
        OLED_Buffer[index] |= (1 << (y % 8));
    else
        OLED_Buffer[index] &= ~(1 << (y % 8));
}

void OLED_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color)
{
    for (uint8_t i = 0; i < h; i++)
        for (uint8_t j = 0; j < w; j++)
            OLED_DrawPixel(x + j, y + i, color);
}

/* ========================== 字符/字符串显示 ========================== */

void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t font_size)
{
    if (font_size != 12) return;  /* 目前仅支持 6x12 (5x7 字体 + 间距) */

    if (ch < 32 || ch > 126) ch = 32;
    uint8_t idx = ch - 32;

    for (uint8_t col = 0; col < 5; col++)
    {
        uint8_t line = font_5x7[idx][col];
        for (uint8_t row = 0; row < 8; row++)
        {
            if (line & (1 << row))
                OLED_DrawPixel(x + col, y + row, 1);
        }
    }
    /* 第6列留空作为字间距 (不画) */
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t font_size)
{
    while (*str)
    {
        OLED_ShowChar(x, y, *str, font_size);
        x += 6;  /* 5 + 1 间距 */
        if (x + 6 > SSD1306_WIDTH) break;
        str++;
    }
}

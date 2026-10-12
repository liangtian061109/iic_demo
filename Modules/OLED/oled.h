#ifndef OLED_H
#define OLED_H

#include "bsp_iic.h"
#include <stdbool.h>
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)
#define OLED_MAX_INSTANCES 1

typedef struct {
    IIC_Instance *iic;
    uint8_t gram[OLED_PAGES][OLED_WIDTH];
} OLED_Instance;

typedef struct {
    IIC_Init_Config_s iic_conf;
} OLED_Init_Config_s;

/* SSD1306、128x64、I2C。注册内部会调用 IIC_Register，应用不要重复注册。
 * 配置在 APP 层提供；此文件不绑定 hi2c1 或设备地址。
 */
OLED_Instance *OLED_Register(const OLED_Init_Config_s *conf);

/* 主循环/任务调用；true 表示全部传输成功，false 表示参数或通信失败。
 * 初始化前须确保屏幕已完成上电等待。具体等待在 main 中安排。
 */
bool OLED_Init(OLED_Instance *oled);
bool OLED_Refresh(OLED_Instance *oled);

/* 绘图接口只修改显存，不发送 I2C。画完一帧后调用一次 OLED_Refresh。
 * 原点在左上角，x 向右、y 向下；超出屏幕的像素自动忽略。
 */
void OLED_Clear(OLED_Instance *oled);
void OLED_Draw_Point(OLED_Instance *oled, uint8_t x, uint8_t y, bool on);
void OLED_Draw_Line(OLED_Instance *oled, uint8_t x1, uint8_t y1,
                    uint8_t x2, uint8_t y2);

/* 固定 8x16 ASCII 字体，支持空格到 '~'；字符串超出右边界即停止。
 * 数字固定宽度并补零，digits 范围 1~10，超过宽度时显示低位数字。
 */
void OLED_Show_Char(OLED_Instance *oled, uint8_t x, uint8_t y, char chr);
void OLED_Show_String(OLED_Instance *oled, uint8_t x, uint8_t y, const char *text);
void OLED_Show_Num(OLED_Instance *oled, uint8_t x, uint8_t y,
                   uint32_t value, uint8_t digits);

#endif

#include "oled.h"
#include "oled_font.h"
#include <stddef.h>
#include <string.h>

static OLED_Instance oled_instances[OLED_MAX_INSTANCES];
static uint8_t oled_instance_count;

static bool OLED_Write_Command(OLED_Instance *oled, uint8_t command)
{
    const uint8_t packet[] = {0x00, command};
    return IIC_Transmit(oled->iic, packet, sizeof(packet)) == HAL_OK;
}

OLED_Instance *OLED_Register(const OLED_Init_Config_s *conf)
{
    if (conf == NULL || oled_instance_count >= OLED_MAX_INSTANCES) {
        return NULL;
    }
    IIC_Instance *iic = IIC_Register(&conf->iic_conf);
    if (iic == NULL) {
        return NULL;
    }
    /* 先完成 OLED 实例初始化，最后提交计数。 */
    const uint8_t instance_index = oled_instance_count;
    OLED_Instance *oled = &oled_instances[instance_index];
    memset(oled, 0, sizeof(*oled));
    oled->iic = iic;
    oled_instance_count = (uint8_t)(instance_index + 1);
    return oled;
}

bool OLED_Init(OLED_Instance *oled)
{
    if (oled == NULL || oled->iic == NULL || IIC_Is_Ready(oled->iic) != HAL_OK) {
        return false;
    }
    /* 首字节 0x00 为命令控制字节；选择页寻址和内部电荷泵。 */
    static const uint8_t init_commands[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F,
        0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x02,
        0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF,
        0xD9, 0xF1, 0xDB, 0x30, 0xA4, 0xA6
    };
    if (IIC_Transmit(oled->iic, init_commands, sizeof(init_commands)) != HAL_OK) {
        return false;
    }
    OLED_Clear(oled);
    /* 先清屏再点亮，避免把上电时的随机显存显示出来。 */
    return OLED_Refresh(oled) && OLED_Write_Command(oled, 0xAF);
}

bool OLED_Refresh(OLED_Instance *oled)
{
    if (oled == NULL || oled->iic == NULL) {
        return false;
    }
    uint8_t packet[OLED_WIDTH + 1];
    packet[0] = 0x40; /* 后续字节为显示数据。 */
    for (uint8_t page = 0; page < OLED_PAGES; ++page) {
        const uint8_t address[] = {0x00, (uint8_t)(0xB0 + page), 0x00, 0x10};
        if (IIC_Transmit(oled->iic, address, sizeof(address)) != HAL_OK) {
            return false;
        }
        memcpy(&packet[1], oled->gram[page], OLED_WIDTH);
        if (IIC_Transmit(oled->iic, packet, sizeof(packet)) != HAL_OK) {
            return false;
        }
    }
    return true;
}

void OLED_Clear(OLED_Instance *oled)
{
    if (oled != NULL) {
        memset(oled->gram, 0, sizeof(oled->gram));
    }
}

void OLED_Draw_Point(OLED_Instance *oled, uint8_t x, uint8_t y, bool on)
{
    if (oled == NULL || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    const uint8_t mask = (uint8_t)(1 << (y % 8));
    if (on) {
        oled->gram[y / 8][x] |= mask;
    } else {
        oled->gram[y / 8][x] &= (uint8_t)~mask;
    }
}

void OLED_Draw_Line(OLED_Instance *oled, uint8_t x1, uint8_t y1,
                    uint8_t x2, uint8_t y2)
{
    if (oled == NULL) {
        return;
    }
    /* Bresenham仅用整数加减即可覆盖水平、竖直和斜线。 */
    int16_t x = x1;
    int16_t y = y1;
    int16_t dx = x2 >= x1 ? x2 - x1 : x1 - x2;
    int16_t dy = y2 >= y1 ? y1 - y2 : y2 - y1;
    int16_t sx = x1 < x2 ? 1 : -1;
    int16_t sy = y1 < y2 ? 1 : -1;
    int16_t error = dx + dy;
    for (;;) {
        OLED_Draw_Point(oled, (uint8_t)x, (uint8_t)y, true);
        if (x == x2 && y == y2) {
            break;
        }
        int16_t twice_error = 2 * error;
        if (twice_error >= dy) {
            error += dy;
            x += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y += sy;
        }
    }
}

void OLED_Show_Char(OLED_Instance *oled, uint8_t x, uint8_t y, char chr)
{
    const uint8_t code = (uint8_t)chr;
    if (oled == NULL || code < 32 || code > 126 || x >= OLED_WIDTH || y >= OLED_HEIGHT) {
        return;
    }
    const uint8_t *glyph = oled_font_8x16[code - 32];
    for (uint8_t row = 0; row < 16 && (uint16_t)y + row < OLED_HEIGHT; ++row) {
        for (uint8_t col = 0; col < 8 && (uint16_t)x + col < OLED_WIDTH; ++col) {
            const uint8_t bits = glyph[(row / 8) * 8 + col];
            OLED_Draw_Point(oled, x + col, y + row, (bits & (1 << (row % 8))) != 0);
        }
    }
}

void OLED_Show_String(OLED_Instance *oled, uint8_t x, uint8_t y, const char *text)
{
    if (oled == NULL || text == NULL || y >= OLED_HEIGHT) {
        return;
    }
    uint16_t cursor_x = x;
    while (*text != '\0' && cursor_x < OLED_WIDTH) {
        OLED_Show_Char(oled, (uint8_t)cursor_x, y, *text++);
        cursor_x += 8;
    }
}

void OLED_Show_Num(OLED_Instance *oled, uint8_t x, uint8_t y,
                   uint32_t value, uint8_t digits)
{
    if (digits == 0 || digits > 10) {
        return;
    }
    char text[11];
    text[digits] = '\0';
    for (uint8_t i = digits; i > 0; --i) {
        text[i - 1] = (char)('0' + value % 10);
        value /= 10;
    }
    OLED_Show_String(oled, x, y, text);
}


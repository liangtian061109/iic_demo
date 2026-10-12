#include "app_oled.h"
#include "i2c.h"
#include "oled.h"

#define APP_OLED_PERIOD_MS 500

/* 设备实例属于 OLED APP，不能暴露给 app_init 或其他 APP。 */
static OLED_Instance *display;
static uint32_t counter;
static uint32_t last_update_ms;
static bool clock_started;

static bool App_OLED_Draw_Frame(void)
{
    OLED_Clear(display);
    OLED_Show_String(display, 0, 0, "OLED I2C DEMO");
    OLED_Show_String(display, 0, 16, "COUNT:");
    OLED_Show_Num(display, 48, 16, counter, 6);
    OLED_Show_String(display, 0, 32, "BSP->MODULE->APP");

    OLED_Draw_Line(display, 0, 51, 127, 51);
    OLED_Draw_Line(display, 0, 63, 127, 63);
    OLED_Draw_Line(display, 0, 51, 0, 63);
    OLED_Draw_Line(display, 127, 51, 127, 63);
    uint8_t progress = (uint8_t)(counter % 126);
    for (uint8_t x = 1; x <= progress; ++x) {
        OLED_Draw_Line(display, x, 54, x, 60);
    }
    return OLED_Refresh(display);
}

bool App_OLED_Init(void)
{
    /* 板级选择属于 APP：OLED 模块本身不绑定 hi2c1 或 0x3C。 */
    const OLED_Init_Config_s oled_conf = {
        .iic_conf = {
            .handle = &hi2c1,
            .dev_addr = 0x3C,
            .timeout_ms = 100
        }
    };

    /* OLED_Register 内部会调用 IIC_Register，APP 不重复注册 I2C。 */
    display = OLED_Register(&oled_conf);
    if (display == NULL || !OLED_Init(display)) {
        display = NULL;
        return false;
    }

    counter = 0;
    last_update_ms = 0;
    clock_started = false;
    return App_OLED_Draw_Frame();
}

bool App_OLED_Update(uint32_t now_ms)
{
    if (display == NULL) {
        return false;
    }
    if (!clock_started) {
        last_update_ms = now_ms;
        clock_started = true;
        return true;
    }
    /* 无符号减法可以正确处理 HAL_GetTick() 回绕。 */
    if ((uint32_t)(now_ms - last_update_ms) < APP_OLED_PERIOD_MS) {
        return true;
    }
    last_update_ms = now_ms;
    counter = (counter + 1) % 1000000;
    return App_OLED_Draw_Frame();
}

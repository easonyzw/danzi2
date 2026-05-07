#include "ui.h"
#include "OLED.h"
#include "Data.h"
#include "gps.h"
#include <stdio.h>

#define UI_REFRESH_INTERVAL_MS      500

static uint32_t last_ui_time = 0;

/* OLED 固定显示一整行，自动补空格，防止旧字符残留 */
static void UI_ShowLine(uint8_t line, char *text)
{
    char buf[17];
    uint8_t i = 0;

    while (i < 16 && text[i] != '\0')
    {
        buf[i] = text[i];
        i++;
    }

    while (i < 16)
    {
        buf[i] = ' ';
        i++;
    }

    buf[16] = '\0';

    OLED_ShowString(line, 1, buf);
}

/* 经纬度 x1000000 转成 x100，也就是保留两位小数 */
static int32_t UI_GpsX1000000_To_X100(int32_t value_x1000000)
{
    if (value_x1000000 >= 0)
    {
        return (value_x1000000 + 5000) / 10000;
    }
    else
    {
        return (value_x1000000 - 5000) / 10000;
    }
}

/* 把带符号 x100 格式化成字符串，例如 4087 -> 40.87 */
static void UI_FormatSignedX100(char *out, int32_t value_x100)
{
    int32_t integer;
    int32_t decimal;

    if (value_x100 < 0)
    {
        value_x100 = -value_x100;
        integer = value_x100 / 100;
        decimal = value_x100 % 100;

        sprintf(out, "-%ld.%02ld", (long)integer, (long)decimal);
    }
    else
    {
        integer = value_x100 / 100;
        decimal = value_x100 % 100;

        sprintf(out, "%ld.%02ld", (long)integer, (long)decimal);
    }
}

void UI_Init(void)
{
    OLED_Clear();
    UI_ShowLine(1, "System Start");
    UI_ShowLine(2, "DHT11 + GPS");
    UI_ShowLine(3, "ESP01S Upload");
    UI_ShowLine(4, "Waiting...");
}

void UI_ShowMainPage(void)
{
    char line[17];
    char lat_str[16];
    char lon_str[16];

    int32_t lat_x100;
    int32_t lon_x100;

    /* 第1行：温湿度 */
    sprintf(line, "T:%dC H:%d%%",
            g_sensor.temperature,
            g_sensor.humidity);
    UI_ShowLine(1, line);

    /* 第2行：GPS状态 */
    if (GPS_Data.valid)
    {
        UI_ShowLine(2, "GPS:OK");
    }
    else
    {
        UI_ShowLine(2, "GPS:NO FIX");
    }

    /* 第3、4行：经纬度，两位小数 */
    if (GPS_Data.valid)
    {
        lat_x100 = UI_GpsX1000000_To_X100(GPS_Data.latitude_x1000000);
        lon_x100 = UI_GpsX1000000_To_X100(GPS_Data.longitude_x1000000);
    }
    else
    {
        lat_x100 = 0;
        lon_x100 = 0;
    }

    UI_FormatSignedX100(lat_str, lat_x100);
    UI_FormatSignedX100(lon_str, lon_x100);

    sprintf(line, "Lat:%s", lat_str);
    UI_ShowLine(3, line);

    sprintf(line, "Lon:%s", lon_str);
    UI_ShowLine(4, line);
}

void UI_Task(uint32_t now)
{
    if (now - last_ui_time >= UI_REFRESH_INTERVAL_MS)
    {
        last_ui_time = now;
        UI_ShowMainPage();
    }
}

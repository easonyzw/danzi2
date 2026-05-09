#include "ui.h"
#include "OLED.h"
#include "data.h"
#include "gps.h"
#include "app.h"
#include <stdio.h>

/*
 * OLED 刷新周期，单位 ms
 * 当前设置为 500ms，也就是 0.5 秒刷新一次 OLED
 */
#define UI_REFRESH_INTERVAL_MS      500

static uint32_t last_ui_time = 0;

/*
 * OLED 固定显示一整行
 * OLED 每行最多显示 16 个字符
 * 不足 16 个字符的地方用空格补齐，防止旧内容残留
 */
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

/*
 * 经纬度 x1000000 转 x100
 * 例如：
 * 40.869150 -> 40869150
 * 40869150 -> 4087
 * OLED 显示为 40.87
 */
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

/*
 * 把 x100 数值格式化为字符串
 * 例如：
 * 4087  -> "40.87"
 * -4087 -> "-40.87"
 */
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

/*
 * 主界面
 *
 * 第1行：温度、湿度
 * 第2行：GPS 状态
 * 第3行：纬度
 * 第4行：经度
 */
void UI_ShowMainPage(void)
{
    char line[17];
    char lat_str[16];
    char lon_str[16];

    int32_t lat_x100;
    int32_t lon_x100;

    sprintf(line, "T:%dC H:%d%%",
            g_sensor.temperature,
            g_sensor.humidity);
    UI_ShowLine(1, line);

    if (GPS_Data.valid)
    {
        UI_ShowLine(2, "GPS:OK");
    }
    else
    {
        UI_ShowLine(2, "GPS:NO FIX");
    }

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

/*
 * 阈值设置界面
 *
 * S16：切换主界面 / 阈值设置界面
 * S15：选择温度阈值 / 湿度阈值
 * S13：加
 * S14：减
 *
 * 当前选中的项前面显示 >
 */
void UI_ShowSettingPage(void)
{
    char line[17];

    UI_ShowLine(1, "Set Threshold");

    if (App_GetSettingItem() == APP_SETTING_TEMP)
    {
        sprintf(line, ">TEMP:%dC", g_setting.temp_threshold);
    }
    else
    {
        sprintf(line, " TEMP:%dC", g_setting.temp_threshold);
    }
    UI_ShowLine(2, line);

    if (App_GetSettingItem() == APP_SETTING_HUMI)
    {
        sprintf(line, ">HUMI:%d%%", g_setting.humi_threshold);
    }
    else
    {
        sprintf(line, " HUMI:%d%%", g_setting.humi_threshold);
    }
    UI_ShowLine(3, line);

    UI_ShowLine(4, "S13+ S14- S15");
}

/*
 * UI 任务函数
 * 每 500ms 刷新一次 OLED
 */
void UI_Task(uint32_t now)
{
    if (now - last_ui_time >= UI_REFRESH_INTERVAL_MS)
    {
        last_ui_time = now;

        if (App_GetPage() == APP_PAGE_SETTING)
        {
            UI_ShowSettingPage();
        }
        else
        {
            UI_ShowMainPage();
        }
    }
}

#include "ui.h"
#include "OLED.h"
#include "Data.h"
#include "gps.h"
#include <stdio.h>

/*
 * OLED 刷新周期，单位 ms
 * 当前设置为 500ms，也就是 0.5 秒刷新一次 OLED
 * 不建议太快，否则 OLED 会闪烁，也会占用太多时间
 */
#define UI_REFRESH_INTERVAL_MS      500

/*
 * 记录上一次 OLED 刷新的时间
 * 配合 Timer_GetMs() 使用，实现非阻塞定时刷新
 */
static uint32_t last_ui_time = 0;

/*
 * OLED 固定显示一整行
 *
 * 作用：
 * OLED 每行最多显示 16 个字符。
 * 如果上一帧显示的是 "GPS:NO FIX"，下一帧只显示 "GPS:OK"，
 * 后面的旧字符可能残留，变成 "GPS:OKFIX"。
 *
 * 所以这里统一把每一行补足 16 个字符，
 * 没有内容的地方用空格填充，防止旧内容残留。
 *
 * 参数：
 * line：OLED 行号，范围 1~4
 * text：要显示的字符串
 */
static void UI_ShowLine(uint8_t line, char *text)
{
    char buf[17];        /* 16 个显示字符 + 1 个字符串结束符 '\0' */
    uint8_t i = 0;

    /*
     * 先复制 text 中的有效字符
     * 最多复制 16 个，因为 OLED 一行最多 16 个字符
     */
    while (i < 16 && text[i] != '\0')
    {
        buf[i] = text[i];
        i++;
    }

    /*
     * 如果 text 不足 16 个字符，
     * 后面用空格补齐，清除上一帧残留字符
     */
    while (i < 16)
    {
        buf[i] = ' ';
        i++;
    }

    /*
     * C 字符串必须以 '\0' 结尾
     */
    buf[16] = '\0';

    /*
     * 从第 1 列开始显示整行内容
     */
    OLED_ShowString(line, 1, buf);
}

/*
 * 经纬度格式转换：
 *
 * GPS_Data.latitude_x1000000 / longitude_x1000000
 * 保存的是经纬度 * 1000000。
 *
 * 例如：
 * 40.869150 会保存为 40869150
 *
 * OLED 当前只显示两位小数：
 * 40.869150 -> 40.87
 *
 * 所以这里把 x1000000 转成 x100。
 *
 * 加 5000 是为了四舍五入：
 * 40869150 + 5000 = 40874150
 * 40874150 / 10000 = 4087
 * 最终显示为 40.87
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
 * 把带符号的 x100 数值格式化为字符串
 *
 * 例如：
 * 4087   -> "40.87"
 * -4087  -> "-40.87"
 *
 * 参数：
 * out：输出字符串缓冲区
 * value_x100：放大 100 倍后的数值
 */
static void UI_FormatSignedX100(char *out, int32_t value_x100)
{
    int32_t integer;
    int32_t decimal;

    /*
     * 处理负数
     * 先取绝对值，再手动添加负号
     */
    if (value_x100 < 0)
    {
        value_x100 = -value_x100;

        integer = value_x100 / 100;     /* 整数部分 */
        decimal = value_x100 % 100;     /* 小数部分 */

        sprintf(out, "-%ld.%02ld", (long)integer, (long)decimal);
    }
    else
    {
        integer = value_x100 / 100;     /* 整数部分 */
        decimal = value_x100 % 100;     /* 小数部分 */

        sprintf(out, "%ld.%02ld", (long)integer, (long)decimal);
    }
}

/*
 * UI 初始化函数
 *
 * 作用：
 * 清空 OLED，并显示启动提示信息。
 *
 * 注意：
 * OLED_Init() 不在这里调用。
 * OLED_Init() 应该放在 Hardware_Init() 里统一初始化硬件。
 */
void UI_Init(void)
{
    OLED_Clear();

    UI_ShowLine(1, "System Start");
    UI_ShowLine(2, "DHT11 + GPS");
    UI_ShowLine(3, "ESP01S Upload");
    UI_ShowLine(4, "Waiting...");
}

/*
 * 显示主界面
 *
 * 当前主界面显示 4 行：
 *
 * 第 1 行：温度和湿度
 * 第 2 行：GPS 定位状态
 * 第 3 行：纬度
 * 第 4 行：经度
 *
 * 示例：
 * T:25C H:60%
 * GPS:OK
 * Lat:40.87
 * Lon:111.68
 */
void UI_ShowMainPage(void)
{
    char line[17];       /* OLED 一行最多 16 字符 */
    char lat_str[16];    /* 纬度字符串 */
    char lon_str[16];    /* 经度字符串 */

    int32_t lat_x100;
    int32_t lon_x100;

    /*
     * 第 1 行：显示温湿度
     *
     * g_sensor.temperature 和 g_sensor.humidity
     * 来自 Data.c 中的全局变量 g_sensor。
     *
     * App_Task() 每 2 秒读取一次 DHT11，
     * 并更新这两个值。
     */
    sprintf(line, "T:%dC H:%d%%",
            g_sensor.temperature,
            g_sensor.humidity);
    UI_ShowLine(1, line);

    /*
     * 第 2 行：显示 GPS 状态
     *
     * GPS_Data.valid = 1：GPS 已定位
     * GPS_Data.valid = 0：GPS 未定位
     */
    if (GPS_Data.valid)
    {
        UI_ShowLine(2, "GPS:OK");
    }
    else
    {
        UI_ShowLine(2, "GPS:NO FIX");
    }

    /*
     * 第 3、4 行：显示经纬度
     *
     * 如果 GPS 已定位：
     * 使用 GPS_Data 中的真实经纬度
     *
     * 如果 GPS 未定位：
     * 显示 0.00，避免显示无效坐标
     */
    if (GPS_Data.valid)
    {
        /*
         * GPS_Data.latitude_x1000000 是纬度 * 1000000
         * GPS_Data.longitude_x1000000 是经度 * 1000000
         *
         * 这里转换成 x100，用于 OLED 显示两位小数
         */
        lat_x100 = UI_GpsX1000000_To_X100(GPS_Data.latitude_x1000000);
        lon_x100 = UI_GpsX1000000_To_X100(GPS_Data.longitude_x1000000);
    }
    else
    {
        lat_x100 = 0;
        lon_x100 = 0;
    }

    /*
     * 把整数形式的经纬度转换成字符串
     *
     * 例如：
     * 4087  -> "40.87"
     * 11168 -> "111.68"
     */
    UI_FormatSignedX100(lat_str, lat_x100);
    UI_FormatSignedX100(lon_str, lon_x100);

    /*
     * 第 3 行：显示纬度
     */
    sprintf(line, "Lat:%s", lat_str);
    UI_ShowLine(3, line);

    /*
     * 第 4 行：显示经度
     */
    sprintf(line, "Lon:%s", lon_str);
    UI_ShowLine(4, line);
}

/*
 * UI 任务函数
 *
 * 这个函数由 App_Task() 循环调用。
 *
 * 作用：
 * 每隔 UI_REFRESH_INTERVAL_MS 刷新一次 OLED。
 *
 * 这种写法不会使用 Delay_ms()，
 * 所以不会长时间阻塞主循环。
 */
void UI_Task(uint32_t now)
{
    /*
     * 判断距离上一次刷新是否已经超过 500ms
     */
    if (now - last_ui_time >= UI_REFRESH_INTERVAL_MS)
    {
        last_ui_time = now;

        /*
         * 当前阶段只有一个主界面，
         * 所以每次刷新都显示主界面。
         *
         * 后面加按键后，可以在这里根据 ui_page
         * 决定显示主界面、阈值界面等。
         */
        UI_ShowMainPage();
    }
}
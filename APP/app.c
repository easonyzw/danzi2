#include "App.h"
#include "Data.h"
#include "ESP01S.h"
#include "Timer.h"
#include "DHT11.h"
#include "gps.h"
#include "ui.h"
#include "Buzzer.h"
#include "Relay.h"
#include "MQ2.h"

/* 温湿度报警阈值 */
#define TEMP_THRESHOLD    30
#define HUMI_THRESHOLD    60

static uint32_t last_dht_time = 0;
static uint32_t last_esp_time = 0;

/* 温湿度报警标志 */
static uint8_t temp_humi_alarm = 0;

/*
 * 报警控制
 *
 * 温度超过 28℃ 或 湿度超过 60%：
 * Relay1 PA5  输出高电平
 * Relay2 PA15 输出高电平
 * Buzzer PA4  输出高电平
 *
 * MQ2 检测到烟雾/燃气：
 * Buzzer PA4 输出高电平
 */
static void App_AlarmControl(void)
{
    uint8_t mq2_alarm = 0;

    if (MQ2_GetState() == MQ2_ALARM)
    {
        mq2_alarm = 1;
    }
    else
    {
        mq2_alarm = 0;
    }

    /*
     * 温湿度报警控制两个继电器
     */
    if (temp_humi_alarm)
    {
        Relay1_Set(1);     // PA5 高电平，继电器1打开
        Relay2_Set(1);     // PA15 高电平，继电器2打开
    }
    else
    {
        Relay1_Set(0);     // PA5 低电平，继电器1关闭
        Relay2_Set(0);     // PA15 低电平，继电器2关闭
    }

    /*
     * 蜂鸣器报警条件：
     * 1. 温湿度超过阈值
     * 2. MQ2 检测到烟雾/燃气
     */
    if (temp_humi_alarm || mq2_alarm)
    {
        Buzzer_Set(1);     // PA4 高电平，蜂鸣器响
    }
    else
    {
        Buzzer_Set(0);     // PA4 低电平，蜂鸣器关闭
    }
}

void App_Init(void)
{
    UI_Init();

    /* 上电默认关闭两个继电器和蜂鸣器 */
    temp_humi_alarm = 0;
    Relay1_Set(0);
    Relay2_Set(0);
    Buzzer_Set(0);
}

void App_Task(void)
{
    uint32_t now = Timer_GetMs();

    GPS_Task();

    if (now - last_dht_time >= 2000)
    {
        last_dht_time = now;

        if (DHT11_ReadData(&g_sensor.temperature, &g_sensor.humidity) == DHT11_OK)
        {
            if (g_sensor.temperature > TEMP_THRESHOLD ||
                g_sensor.humidity > HUMI_THRESHOLD)
            {
                temp_humi_alarm = 1;
            }
            else
            {
                temp_humi_alarm = 0;
            }
        }
        else
        {
            /*
             * DHT11 读取失败时，取消温湿度报警，避免误触发继电器
             * 但 MQ2 烟雾报警仍然会继续检测
             */
            temp_humi_alarm = 0;
        }
    }

    /*
     * 每次循环都检查 MQ2
     * 这样检测到烟雾后蜂鸣器能更快响应，不用等 DHT11 的 2 秒周期
     */
    App_AlarmControl();

    UI_Task(now);

    if (now - last_esp_time >= 5000)
    {
        last_esp_time = now;

        ESP01S_SendData(g_sensor.temperature,
                        g_sensor.humidity,
                        GPS_Data.latitude_x1000000,
                        GPS_Data.longitude_x1000000,
                        GPS_Data.altitude_x100,
                        GPS_Data.valid);
    }
}

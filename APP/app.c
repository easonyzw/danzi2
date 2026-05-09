#include "App.h"
#include "data.h"
#include "ESP01S.h"
#include "Timer.h"
#include "DHT11.h"
#include "gps.h"
#include "ui.h"
#include "Buzzer.h"
#include "Relay.h"
#include "MQ2.h"
#include "Key.h"

/* 吸合/蜂鸣保持时间：5秒 */
#define ALARM_ON_TIME_MS        5000

/* 阈值范围限制 */
#define TEMP_THRESHOLD_MIN      0
#define TEMP_THRESHOLD_MAX      99
#define HUMI_THRESHOLD_MIN      0
#define HUMI_THRESHOLD_MAX      100

static uint32_t last_dht_time = 0;
static uint32_t last_esp_time = 0;

/* 当前报警状态 */
static uint8_t temp_alarm = 0;
static uint8_t humi_alarm = 0;
static uint8_t mq2_alarm = 0;

/* 上一次报警状态，用来判断“刚刚检测到” */
static uint8_t last_temp_alarm = 0;
static uint8_t last_humi_alarm = 0;
static uint8_t last_mq2_alarm = 0;

/* 5秒定时控制 */
static uint8_t relay1_timer_active = 0;
static uint8_t relay2_timer_active = 0;
static uint8_t buzzer_timer_active = 0;

static uint32_t relay1_start_time = 0;
static uint32_t relay2_start_time = 0;
static uint32_t buzzer_start_time = 0;

/* 当前页面 */
static uint8_t app_page = APP_PAGE_MAIN;

/* 当前阈值设置项 */
static uint8_t app_setting_item = APP_SETTING_TEMP;

/*
 * 下面两个函数后面给 ui.c 使用
 * ui.c 可以通过这两个函数知道：
 * 当前显示主界面还是阈值设置界面
 * 当前选中温度阈值还是湿度阈值
 */
uint8_t App_GetPage(void)
{
    return app_page;
}

uint8_t App_GetSettingItem(void)
{
    return app_setting_item;
}

/*
 * 按键处理
 *
 * S16：切换主界面 / 阈值设置界面
 * S15：在阈值设置界面中，选择温度阈值 / 湿度阈值
 * S13：当前选中阈值 +1
 * S14：当前选中阈值 -1
 */
static void App_KeyTask(void)
{
    uint8_t key;

    key = Key_GetNum();

    if (key == 0)
    {
        return;
    }

    /*
     * S16：切换界面
     */
    if (key == 16)
    {
        if (app_page == APP_PAGE_MAIN)
        {
            app_page = APP_PAGE_SETTING;
        }
        else
        {
            app_page = APP_PAGE_MAIN;
        }
    }

    /*
     * 下面这些按键只在阈值设置界面有效
     */
    if (app_page != APP_PAGE_SETTING)
    {
        return;
    }

    /*
     * S15：选择温度阈值 / 湿度阈值
     */
    if (key == 15)
    {
        if (app_setting_item == APP_SETTING_TEMP)
        {
            app_setting_item = APP_SETTING_HUMI;
        }
        else
        {
            app_setting_item = APP_SETTING_TEMP;
        }
    }

    /*
     * S13：当前选中的阈值 +1
     */
    else if (key == 13)
    {
        if (app_setting_item == APP_SETTING_TEMP)
        {
            if (g_setting.temp_threshold < TEMP_THRESHOLD_MAX)
            {
                g_setting.temp_threshold++;
            }
        }
        else
        {
            if (g_setting.humi_threshold < HUMI_THRESHOLD_MAX)
            {
                g_setting.humi_threshold++;
            }
        }
    }

    /*
     * S14：当前选中的阈值 -1
     */
    else if (key == 14)
    {
        if (app_setting_item == APP_SETTING_TEMP)
        {
            if (g_setting.temp_threshold > TEMP_THRESHOLD_MIN)
            {
                g_setting.temp_threshold--;
            }
        }
        else
        {
            if (g_setting.humi_threshold > HUMI_THRESHOLD_MIN)
            {
                g_setting.humi_threshold--;
            }
        }
    }
}

/*
 * 报警控制逻辑：
 *
 * 温度 > g_setting.temp_threshold：
 * Relay1 PA5 高电平吸合 5 秒
 *
 * 湿度 > g_setting.humi_threshold：
 * Relay2 PA15 高电平吸合 5 秒
 *
 * MQ2 检测到烟雾：
 * Buzzer PA4 高电平响 5 秒
 */
static void App_AlarmControl(uint32_t now)
{
    /*
     * 温度刚刚超标：继电器1吸合5秒
     */
    if (temp_alarm && !last_temp_alarm)
    {
        Relay1_Set(1);                 // PA5 高电平，继电器1吸合
        relay1_timer_active = 1;
        relay1_start_time = now;
    }

    /*
     * 湿度刚刚超标：继电器2吸合5秒
     */
    if (humi_alarm && !last_humi_alarm)
    {
        Relay2_Set(1);                 // PA15 高电平，继电器2吸合
        relay2_timer_active = 1;
        relay2_start_time = now;
    }

    /*
     * MQ2 刚刚检测到烟雾：蜂鸣器响5秒
     */
    if (mq2_alarm && !last_mq2_alarm)
    {
        Buzzer_Set(1);                 // PA4 高电平，蜂鸣器响
        buzzer_timer_active = 1;
        buzzer_start_time = now;
    }

    last_temp_alarm = temp_alarm;
    last_humi_alarm = humi_alarm;
    last_mq2_alarm = mq2_alarm;

    /*
     * 继电器1吸合满5秒后断开
     */
    if (relay1_timer_active)
    {
        if (now - relay1_start_time >= ALARM_ON_TIME_MS)
        {
            Relay1_Set(0);             // PA5 低电平，继电器1断开
            relay1_timer_active = 0;
        }
    }

    /*
     * 继电器2吸合满5秒后断开
     */
    if (relay2_timer_active)
    {
        if (now - relay2_start_time >= ALARM_ON_TIME_MS)
        {
            Relay2_Set(0);             // PA15 低电平，继电器2断开
            relay2_timer_active = 0;
        }
    }

    /*
     * 蜂鸣器响满5秒后关闭
     */
    if (buzzer_timer_active)
    {
        if (now - buzzer_start_time >= ALARM_ON_TIME_MS)
        {
            Buzzer_Set(0);             // PA4 低电平，蜂鸣器关闭
            buzzer_timer_active = 0;
        }
    }
}

void App_Init(void)
{
    UI_Init();

    temp_alarm = 0;
    humi_alarm = 0;
    mq2_alarm = 0;

    last_temp_alarm = 0;
    last_humi_alarm = 0;
    last_mq2_alarm = 0;

    relay1_timer_active = 0;
    relay2_timer_active = 0;
    buzzer_timer_active = 0;

    app_page = APP_PAGE_MAIN;
    app_setting_item = APP_SETTING_TEMP;

    /* 上电默认关闭继电器和蜂鸣器 */
    Relay1_Set(0);
    Relay2_Set(0);
    Buzzer_Set(0);
}

void App_Task(void)
{
    uint32_t now = Timer_GetMs();

    GPS_Task();

    /*
     * 按键处理
     */
    App_KeyTask();

    /*
     * 每2秒读取一次温湿度
     */
    if (now - last_dht_time >= 2000)
    {
        last_dht_time = now;

        if (DHT11_ReadData(&g_sensor.temperature, &g_sensor.humidity) == DHT11_OK)
        {
            if (g_sensor.temperature > g_setting.temp_threshold)
            {
                temp_alarm = 1;
            }
            else
            {
                temp_alarm = 0;
            }

            if (g_sensor.humidity > g_setting.humi_threshold)
            {
                humi_alarm = 1;
            }
            else
            {
                humi_alarm = 0;
            }
        }
        else
        {
            /*
             * DHT11 读取失败时，取消温湿度报警
             */
            temp_alarm = 0;
            humi_alarm = 0;
        }
    }

    /*
     * MQ2 每次循环都检测
     * PA1 低电平 = 检测到烟雾
     */
    if (MQ2_GetState() == MQ2_ALARM)
    {
        mq2_alarm = 1;
    }
    else
    {
        mq2_alarm = 0;
    }

    /*
     * 执行5秒报警控制
     */
    App_AlarmControl(now);

    /*
     * OLED 显示任务
     */
    UI_Task(now);

    /*
     * 每5秒上传一次数据
     */
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

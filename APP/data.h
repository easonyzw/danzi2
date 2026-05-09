#ifndef __DATA_H
#define __DATA_H

#include "stm32f10x.h"

/*
 * 传感器数据结构体
 * 保存当前 DHT11 读取到的温湿度
 */
typedef struct
{
    uint8_t temperature;     // 当前温度
    uint8_t humidity;        // 当前湿度
} SensorData_t;

/*
 * 系统设置结构体
 * 保存可以通过按键修改的阈值
 */
typedef struct
{
    uint8_t temp_threshold;  // 温度报警阈值
    uint8_t humi_threshold;  // 湿度报警阈值
} SystemSetting_t;

/*
 * 全局变量声明
 */
extern SensorData_t g_sensor;
extern SystemSetting_t g_setting;

#endif

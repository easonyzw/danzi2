#include "Data.h"

/*
 * 当前传感器数据
 * 初始值给 0
 */
SensorData_t g_sensor =
{
    0,      // temperature
    0       // humidity
};

/*
 * 系统设置参数
 * 默认温度阈值：30℃
 * 默认湿度阈值：60%
 */
SystemSetting_t g_setting =
{
    30,     // temp_threshold
    60      // humi_threshold
};

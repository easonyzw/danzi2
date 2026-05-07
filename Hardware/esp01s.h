#ifndef __ESP01S_H
#define __ESP01S_H

#include "stm32f10x.h"

void ESP01S_Init(void);
void ESP01S_SendByte(uint8_t byte);
void ESP01S_SendString(char *str);

/*
 * 发送给 ESP01S 的数据格式：
 *
 * {
 *   "EnvironmentTemperature":25.0,
 *   "EnvironmentHumidity":60.0,
 *   "Latitude":40.87,
 *   "Longitude":111.68,
 *   "Altitude":0.00
 * }
 *
 * 注意：
 * latitude_x1000000  = 纬度 * 1000000
 * longitude_x1000000 = 经度 * 1000000
 * altitude_x100      = 海拔 * 100
 */
void ESP01S_SendData(uint8_t temperature,
                     uint8_t humidity,
                     int32_t latitude_x1000000,
                     int32_t longitude_x1000000,
                     int32_t altitude_x100,
                     uint8_t gps_valid);

#endif

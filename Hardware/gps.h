#ifndef __GPS_H
#define __GPS_H

#include "stm32f10x.h"
#include <stdint.h>

/*
 * GPS 默认配置
 *
 * 接线：
 * GPS_TXD -> STM32 PA10 / USART1_RX
 * GPS_RXD -> STM32 PA9  / USART1_TX，可不接
 * GND     -> GND
 * VCC     -> 按模块要求接 3.3V 或 5V
 *
 * 默认波特率：9600
 */
#define GPS_DEFAULT_BAUDRATE    9600

/*
 * 默认关闭 PA9 TX 输出。
 * 只接收 GPS 数据时，GPS_RXD 可以不接 PA9。
 * 如果后面需要给 GPS 发送配置命令，可以改成 1。
 */
#define GPS_ENABLE_TX           0

#define GPS_RX_BUFFER_LEN       128

#define GPS_FIELD_UTC_LEN       16
#define GPS_FIELD_DATE_LEN      12
#define GPS_FIELD_RAW_LEN       20
#define GPS_FIELD_DEC_LEN       20
#define GPS_FIELD_ALT_LEN       16

typedef struct
{
    volatile uint8_t rmc_ready;
    volatile uint8_t gga_ready;

    uint8_t parsed;
    uint8_t valid;              /* 1=定位有效，0=未定位 */

    uint8_t fix_quality;        /* GGA 定位质量，0=无定位，1=GPS定位 */
    uint8_t satellites;         /* 卫星数量 */

    char utc[GPS_FIELD_UTC_LEN];
    char date[GPS_FIELD_DATE_LEN];

    char latitude_raw[GPS_FIELD_RAW_LEN];     /* 原始纬度 ddmm.mmmm */
    char longitude_raw[GPS_FIELD_RAW_LEN];    /* 原始经度 dddmm.mmmm */
    char ns[4];                               /* N/S */
    char ew[4];                               /* E/W */

    char latitude[GPS_FIELD_DEC_LEN];         /* 小数纬度，例如 40.869150 */
    char longitude[GPS_FIELD_DEC_LEN];        /* 小数经度，例如 111.680231 */

    char altitude_raw[GPS_FIELD_ALT_LEN];     /* 原始海拔字符串 */
    char altitude[GPS_FIELD_ALT_LEN];         /* 小数海拔，例如 1060.50 */

    int32_t latitude_x1000000;                /* 纬度 * 1000000 */
    int32_t longitude_x1000000;               /* 经度 * 1000000 */
    int32_t altitude_x100;                    /* 海拔 * 100，单位 m */
} GPS_Data_t;

extern GPS_Data_t GPS_Data;

void GPS_Init(void);
void GPS_InitBaud(uint32_t baudrate);
void GPS_Clear(void);

/*
 * 在主循环里调用。
 * 有新 GPS 数据被解析返回 1，否则返回 0。
 */
uint8_t GPS_Task(void);
uint8_t GPS_Parse(void);

uint8_t GPS_IsValid(void);

char *GPS_GetLatitudeString(void);
char *GPS_GetLongitudeString(void);
char *GPS_GetAltitudeString(void);

int32_t GPS_GetLatitudeX1000000(void);
int32_t GPS_GetLongitudeX1000000(void);
int32_t GPS_GetAltitudeX100(void);

#endif

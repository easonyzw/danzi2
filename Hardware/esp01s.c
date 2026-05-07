#include "ESP01S.h"
#include <stdio.h>

void ESP01S_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* 开启 GPIOA 和 USART2 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    /* PA2 -> USART2_TX -> 接 ESP-01S RX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PA3 -> USART2_RX -> 接 ESP-01S TX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* USART2 参数配置 */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

void ESP01S_SendByte(uint8_t byte)
{
    USART_SendData(USART2, byte);

    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void ESP01S_SendString(char *str)
{
    while (*str)
    {
        ESP01S_SendByte(*str++);
    }
}

/*
 * 把 x1000000 的经纬度转换成 x100，两位小数。
 *
 * 例如：
 * 40.869150 * 1000000 = 40869150
 * 转成两位小数：
 * 40869150 / 10000 = 4086
 * 显示为：
 * 40.86
 *
 * 加 5000 是为了四舍五入：
 * 40.869150 -> 40.87
 */
static int32_t ESP01S_GpsX1000000_To_X100(int32_t value_x1000000)
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
 * 把带符号的 x100 数字拆成整数部分和小数部分
 *
 * 例如：
 * 4087 -> 40.87
 * -4087 -> -40.87
 */
static void ESP01S_FormatSignedX100(char *out, int32_t value_x100)
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

/*
 * 发送温湿度 + GPS 坐标给 ESP01S
 *
 * OneNet 字段：
 * EnvironmentTemperature
 * EnvironmentHumidity
 * Latitude
 * Longitude
 * Altitude
 *
 * 注意：
 * Latitude / Longitude / Altitude 都按两位小数发送，适配步长 0.01。
 */
void ESP01S_SendData(uint8_t temperature,
                     uint8_t humidity,
                     int32_t latitude_x1000000,
                     int32_t longitude_x1000000,
                     int32_t altitude_x100,
                     uint8_t gps_valid)
{
    char buffer[180];

    char lat_str[20];
    char lon_str[20];
    char alt_str[20];

    int32_t lat_x100;
    int32_t lon_x100;

    if (gps_valid)
    {
        lat_x100 = ESP01S_GpsX1000000_To_X100(latitude_x1000000);
        lon_x100 = ESP01S_GpsX1000000_To_X100(longitude_x1000000);
    }
    else
    {
        lat_x100 = 0;
        lon_x100 = 0;
        altitude_x100 = 0;
    }

    ESP01S_FormatSignedX100(lat_str, lat_x100);
    ESP01S_FormatSignedX100(lon_str, lon_x100);
    ESP01S_FormatSignedX100(alt_str, altitude_x100);

    sprintf(buffer,
            "{\"EnvironmentTemperature\":%d.0,"
            "\"EnvironmentHumidity\":%d.0,"
            "\"Latitude\":%s,"
            "\"Longitude\":%s,"
            "\"Altitude\":%s}\r\n",
            temperature,
            humidity,
            lat_str,
            lon_str,
            alt_str);

    ESP01S_SendString(buffer);
}

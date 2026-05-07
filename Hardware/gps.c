#include "gps.h"
#include <string.h>
#include <stdio.h>

GPS_Data_t GPS_Data;

static char GPS_RxBuf[GPS_RX_BUFFER_LEN];
static volatile uint16_t GPS_RxIndex = 0;

static char GPS_RMC_Raw[GPS_RX_BUFFER_LEN];
static char GPS_GGA_Raw[GPS_RX_BUFFER_LEN];

/* 判断是否为 $GPRMC / $GNRMC */
static uint8_t GPS_IsRMC(const char *buf)
{
    if (buf[0] == '$' &&
        buf[3] == 'R' &&
        buf[4] == 'M' &&
        buf[5] == 'C')
    {
        return 1;
    }

    return 0;
}

/* 判断是否为 $GPGGA / $GNGGA */
static uint8_t GPS_IsGGA(const char *buf)
{
    if (buf[0] == '$' &&
        buf[3] == 'G' &&
        buf[4] == 'G' &&
        buf[5] == 'A')
    {
        return 1;
    }

    return 0;
}

/*
 * 从 NMEA 语句中提取指定字段
 *
 * 例如 RMC：
 * 0: $GNRMC
 * 1: UTC
 * 2: A/V
 * 3: 纬度
 * 4: N/S
 * 5: 经度
 * 6: E/W
 * 7: 速度
 * 8: 航向
 * 9: 日期
 *
 * 例如 GGA：
 * 0: $GNGGA
 * 1: UTC
 * 2: 纬度
 * 3: N/S
 * 4: 经度
 * 5: E/W
 * 6: 定位质量
 * 7: 卫星数
 * 8: HDOP
 * 9: 海拔
 */
static void GPS_CopyField(const char *sentence,
                          uint8_t field_index,
                          char *out,
                          uint8_t out_len)
{
    uint8_t field = 0;
    const char *start = sentence;
    const char *p = sentence;
    uint16_t len;
    uint16_t i;

    if (out_len == 0)
    {
        return;
    }

    out[0] = '\0';

    while (1)
    {
        if (*p == ',' || *p == '*' || *p == '\r' || *p == '\n' || *p == '\0')
        {
            if (field == field_index)
            {
                len = p - start;

                if (len >= out_len)
                {
                    len = out_len - 1;
                }

                for (i = 0; i < len; i++)
                {
                    out[i] = start[i];
                }

                out[len] = '\0';
                return;
            }

            if (*p == '*' || *p == '\r' || *p == '\n' || *p == '\0')
            {
                return;
            }

            field++;
            start = p + 1;
        }

        p++;
    }
}

static uint8_t GPS_StrToU8(const char *str)
{
    uint8_t value = 0;

    while (*str >= '0' && *str <= '9')
    {
        value = value * 10 + (*str - '0');
        str++;
    }

    return value;
}

/*
 * 把 NMEA 经纬度转换成 小数度 * 1000000
 *
 * 纬度格式：ddmm.mmmmm
 * 经度格式：dddmm.mmmmm
 *
 * 例如：
 * 4052.14902,N -> 40.869150
 * 11140.81387,E -> 111.680231
 */
static uint8_t GPS_NMEA_ToDecimalX1000000(const char *raw,
                                          char hemi,
                                          uint8_t is_longitude,
                                          int32_t *out)
{
    uint8_t deg_digits;
    uint8_t i;
    int32_t degree = 0;
    int32_t minute_int = 0;
    int32_t minute_frac = 0;
    int32_t frac_base = 100000;
    int32_t minute_x1000000;
    int32_t result;
    const char *p;

    if (raw == 0 || raw[0] == '\0' || out == 0)
    {
        return 0;
    }

    deg_digits = is_longitude ? 3 : 2;

    for (i = 0; i < deg_digits; i++)
    {
        if (raw[i] < '0' || raw[i] > '9')
        {
            return 0;
        }

        degree = degree * 10 + (raw[i] - '0');
    }

    p = &raw[deg_digits];

    while (*p >= '0' && *p <= '9')
    {
        minute_int = minute_int * 10 + (*p - '0');
        p++;
    }

    if (*p == '.')
    {
        p++;

        while (*p >= '0' && *p <= '9' && frac_base > 0)
        {
            minute_frac += (*p - '0') * frac_base;
            frac_base /= 10;
            p++;
        }
    }

    minute_x1000000 = minute_int * 1000000 + minute_frac;

    result = degree * 1000000 + minute_x1000000 / 60;

    if (hemi == 'S' || hemi == 'W')
    {
        result = -result;
    }

    *out = result;

    return 1;
}

/* 把 x1000000 格式化成字符串，例如 40869150 -> "40.869150" */
static void GPS_FormatDecimal6(char *out, int32_t value)
{
    int32_t integer;
    int32_t frac;

    if (value < 0)
    {
        value = -value;
        integer = value / 1000000;
        frac = value % 1000000;
        sprintf(out, "-%ld.%06ld", integer, frac);
    }
    else
    {
        integer = value / 1000000;
        frac = value % 1000000;
        sprintf(out, "%ld.%06ld", integer, frac);
    }
}

/*
 * 把海拔字符串转换成 x100
 * 例如 "1060.5" -> 106050
 */
static uint8_t GPS_AltitudeToX100(const char *raw, int32_t *out)
{
    int32_t sign = 1;
    int32_t integer = 0;
    int32_t frac = 0;
    uint8_t frac_count = 0;

    if (raw == 0 || raw[0] == '\0' || out == 0)
    {
        return 0;
    }

    if (*raw == '-')
    {
        sign = -1;
        raw++;
    }

    while (*raw >= '0' && *raw <= '9')
    {
        integer = integer * 10 + (*raw - '0');
        raw++;
    }

    if (*raw == '.')
    {
        raw++;

        while (*raw >= '0' && *raw <= '9' && frac_count < 2)
        {
            frac = frac * 10 + (*raw - '0');
            frac_count++;
            raw++;
        }
    }

    while (frac_count < 2)
    {
        frac *= 10;
        frac_count++;
    }

    *out = sign * (integer * 100 + frac);

    return 1;
}

/* 把 x100 格式化成字符串，例如 106050 -> "1060.50" */
static void GPS_FormatDecimal2(char *out, int32_t value)
{
    int32_t integer;
    int32_t frac;

    if (value < 0)
    {
        value = -value;
        integer = value / 100;
        frac = value % 100;
        sprintf(out, "-%ld.%02ld", integer, frac);
    }
    else
    {
        integer = value / 100;
        frac = value % 100;
        sprintf(out, "%ld.%02ld", integer, frac);
    }
}

void GPS_Clear(void)
{
    GPS_RxIndex = 0;

    memset(GPS_RxBuf, 0, GPS_RX_BUFFER_LEN);
    memset(GPS_RMC_Raw, 0, GPS_RX_BUFFER_LEN);
    memset(GPS_GGA_Raw, 0, GPS_RX_BUFFER_LEN);
    memset(&GPS_Data, 0, sizeof(GPS_Data));

    strcpy(GPS_Data.latitude, "0.000000");
    strcpy(GPS_Data.longitude, "0.000000");
    strcpy(GPS_Data.altitude, "0.00");
}

void GPS_Init(void)
{
    GPS_InitBaud(GPS_DEFAULT_BAUDRATE);
}

void GPS_InitBaud(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    GPS_Clear();

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

#if GPS_ENABLE_TX
    /* PA9 -> USART1_TX，可选 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
#endif

    /* PA10 -> USART1_RX，GPS_TXD 接这里 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

#if GPS_ENABLE_TX
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
#else
    USART_InitStructure.USART_Mode = USART_Mode_Rx;
#endif

    USART_Init(USART1, &USART_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

void USART1_IRQHandler(void)
{
    uint8_t ch;
    uint16_t i;
    uint16_t copy_len;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        ch = (uint8_t)USART_ReceiveData(USART1);

        if (ch == '$')
        {
            GPS_RxIndex = 0;
        }

        if (GPS_RxIndex < GPS_RX_BUFFER_LEN - 1)
        {
            GPS_RxBuf[GPS_RxIndex++] = ch;
        }
        else
        {
            GPS_RxIndex = 0;
        }

        if (ch == '\n')
        {
            GPS_RxBuf[GPS_RxIndex] = '\0';

            copy_len = GPS_RxIndex;

            if (copy_len >= GPS_RX_BUFFER_LEN)
            {
                copy_len = GPS_RX_BUFFER_LEN - 1;
            }

            if (GPS_IsRMC(GPS_RxBuf))
            {
                for (i = 0; i < copy_len; i++)
                {
                    GPS_RMC_Raw[i] = GPS_RxBuf[i];
                }

                GPS_RMC_Raw[copy_len] = '\0';
                GPS_Data.rmc_ready = 1;
            }
            else if (GPS_IsGGA(GPS_RxBuf))
            {
                for (i = 0; i < copy_len; i++)
                {
                    GPS_GGA_Raw[i] = GPS_RxBuf[i];
                }

                GPS_GGA_Raw[copy_len] = '\0';
                GPS_Data.gga_ready = 1;
            }

            GPS_RxIndex = 0;
        }
    }
}

static void GPS_ParseRMC(const char *sentence)
{
    char status[4];

    GPS_CopyField(sentence, 1, GPS_Data.utc, GPS_FIELD_UTC_LEN);
    GPS_CopyField(sentence, 2, status, sizeof(status));
    GPS_CopyField(sentence, 3, GPS_Data.latitude_raw, GPS_FIELD_RAW_LEN);
    GPS_CopyField(sentence, 4, GPS_Data.ns, sizeof(GPS_Data.ns));
    GPS_CopyField(sentence, 5, GPS_Data.longitude_raw, GPS_FIELD_RAW_LEN);
    GPS_CopyField(sentence, 6, GPS_Data.ew, sizeof(GPS_Data.ew));
    GPS_CopyField(sentence, 9, GPS_Data.date, GPS_FIELD_DATE_LEN);

    if (status[0] == 'A')
    {
        GPS_Data.valid = 1;
    }
    else
    {
        GPS_Data.valid = 0;
    }

    if (GPS_NMEA_ToDecimalX1000000(GPS_Data.latitude_raw,
                                    GPS_Data.ns[0],
                                    0,
                                    &GPS_Data.latitude_x1000000))
    {
        GPS_FormatDecimal6(GPS_Data.latitude, GPS_Data.latitude_x1000000);
    }

    if (GPS_NMEA_ToDecimalX1000000(GPS_Data.longitude_raw,
                                    GPS_Data.ew[0],
                                    1,
                                    &GPS_Data.longitude_x1000000))
    {
        GPS_FormatDecimal6(GPS_Data.longitude, GPS_Data.longitude_x1000000);
    }
}

static void GPS_ParseGGA(const char *sentence)
{
    char quality[4];
    char satellites[4];

    GPS_CopyField(sentence, 1, GPS_Data.utc, GPS_FIELD_UTC_LEN);
    GPS_CopyField(sentence, 2, GPS_Data.latitude_raw, GPS_FIELD_RAW_LEN);
    GPS_CopyField(sentence, 3, GPS_Data.ns, sizeof(GPS_Data.ns));
    GPS_CopyField(sentence, 4, GPS_Data.longitude_raw, GPS_FIELD_RAW_LEN);
    GPS_CopyField(sentence, 5, GPS_Data.ew, sizeof(GPS_Data.ew));
    GPS_CopyField(sentence, 6, quality, sizeof(quality));
    GPS_CopyField(sentence, 7, satellites, sizeof(satellites));
    GPS_CopyField(sentence, 9, GPS_Data.altitude_raw, GPS_FIELD_ALT_LEN);

    GPS_Data.fix_quality = GPS_StrToU8(quality);
    GPS_Data.satellites = GPS_StrToU8(satellites);

    if (GPS_Data.fix_quality > 0)
    {
        GPS_Data.valid = 1;
    }

    if (GPS_NMEA_ToDecimalX1000000(GPS_Data.latitude_raw,
                                    GPS_Data.ns[0],
                                    0,
                                    &GPS_Data.latitude_x1000000))
    {
        GPS_FormatDecimal6(GPS_Data.latitude, GPS_Data.latitude_x1000000);
    }

    if (GPS_NMEA_ToDecimalX1000000(GPS_Data.longitude_raw,
                                    GPS_Data.ew[0],
                                    1,
                                    &GPS_Data.longitude_x1000000))
    {
        GPS_FormatDecimal6(GPS_Data.longitude, GPS_Data.longitude_x1000000);
    }

    if (GPS_AltitudeToX100(GPS_Data.altitude_raw, &GPS_Data.altitude_x100))
    {
        GPS_FormatDecimal2(GPS_Data.altitude, GPS_Data.altitude_x100);
    }
}

uint8_t GPS_Parse(void)
{
    char rmc_sentence[GPS_RX_BUFFER_LEN];
    char gga_sentence[GPS_RX_BUFFER_LEN];
    uint8_t has_rmc = 0;
    uint8_t has_gga = 0;

    USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);

    if (GPS_Data.rmc_ready)
    {
        memcpy(rmc_sentence, GPS_RMC_Raw, GPS_RX_BUFFER_LEN);
        GPS_Data.rmc_ready = 0;
        has_rmc = 1;
    }

    if (GPS_Data.gga_ready)
    {
        memcpy(gga_sentence, GPS_GGA_Raw, GPS_RX_BUFFER_LEN);
        GPS_Data.gga_ready = 0;
        has_gga = 1;
    }

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    if (has_rmc)
    {
        GPS_ParseRMC(rmc_sentence);
    }

    if (has_gga)
    {
        GPS_ParseGGA(gga_sentence);
    }

    if (has_rmc || has_gga)
    {
        GPS_Data.parsed = 1;
        return 1;
    }

    return 0;
}

uint8_t GPS_Task(void)
{
    return GPS_Parse();
}

uint8_t GPS_IsValid(void)
{
    return GPS_Data.valid;
}

char *GPS_GetLatitudeString(void)
{
    return GPS_Data.latitude;
}

char *GPS_GetLongitudeString(void)
{
    return GPS_Data.longitude;
}

char *GPS_GetAltitudeString(void)
{
    return GPS_Data.altitude;
}

int32_t GPS_GetLatitudeX1000000(void)
{
    return GPS_Data.latitude_x1000000;
}

int32_t GPS_GetLongitudeX1000000(void)
{
    return GPS_Data.longitude_x1000000;
}

int32_t GPS_GetAltitudeX100(void)
{
    return GPS_Data.altitude_x100;
}

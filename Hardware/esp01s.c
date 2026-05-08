#include "ESP01S.h"
#include <stdio.h>

/*
 * ESP01S 初始化函数
 *
 * 功能：
 * 1. 初始化 USART2
 * 2. 配置 PA2 为 USART2_TX
 * 3. 配置 PA3 为 USART2_RX
 *
 * 接线：
 * STM32 PA2 / USART2_TX -> ESP01S RX
 * STM32 PA3 / USART2_RX -> ESP01S TX
 *
 * 注意：
 * ESP01S 一般使用 115200 波特率。
 */
void ESP01S_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /*
     * 开启 GPIOA 和 USART2 时钟
     *
     * PA2、PA3 属于 GPIOA
     * USART2 挂在 APB1 总线上
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    /*
     * PA2 -> USART2_TX
     *
     * PA2 是 STM32 的串口发送脚，
     * 要接到 ESP01S 的 RX。
     *
     * GPIO_Mode_AF_PP 表示复用推挽输出，
     * 串口 TX 必须配置成这个模式。
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * PA3 -> USART2_RX
     *
     * PA3 是 STM32 的串口接收脚，
     * 要接到 ESP01S 的 TX。
     *
     * GPIO_Mode_IPU 表示上拉输入。
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * USART2 参数配置
     *
     * 115200：波特率
     * 8b：8 位数据位
     * 1 stop bit：1 位停止位
     * No parity：无校验
     * No flow control：无硬件流控
     * Tx | Rx：允许发送和接收
     */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    /*
     * 初始化 USART2，并使能 USART2
     */
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

/*
 * 通过 USART2 发送 1 个字节
 *
 * 参数：
 * byte：要发送的一个字节数据
 *
 * 说明：
 * USART_SendData() 把数据写入 USART2 的发送数据寄存器。
 * TXE 标志位表示发送数据寄存器为空，
 * 等它为空后，才能继续发送下一个字节。
 */
void ESP01S_SendByte(uint8_t byte)
{
    /*
     * 把一个字节写入 USART2 发送寄存器
     */
    USART_SendData(USART2, byte);

    /*
     * 等待发送数据寄存器为空
     *
     * 这是一种阻塞式发送方式。
     * 也就是说：当前字节没有准备好发送完之前，
     * 程序会停在这里等待。
     *
     * 由于一条 JSON 数据不长，所以这个阻塞时间很短。
     */
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

/*
 * 通过 USART2 发送字符串
 *
 * 参数：
 * str：要发送的字符串
 *
 * 说明：
 * C 语言字符串以 '\0' 结尾。
 * while (*str) 表示只要当前字符不是 '\0'，
 * 就继续发送。
 *
 * 例如：
 * ESP01S_SendString("ABC");
 *
 * 实际发送：
 * 'A' -> 'B' -> 'C'
 * 遇到 '\0' 后结束。
 */
void ESP01S_SendString(char *str)
{
    while (*str)
    {
        /*
         * *str 表示当前字符
         * str++ 表示发送完当前字符后，指针移动到下一个字符
         *
         * 这句等价于：
         * ESP01S_SendByte(*str);
         * str++;
         */
        ESP01S_SendByte(*str++);
    }
}

/*
 * 把 x1000000 的经纬度转换成 x100，两位小数
 *
 * GPS_Data 里保存的经纬度不是 float，
 * 而是把真实值放大 1000000 倍后保存成整数。
 *
 * 例如：
 * 真实纬度：40.869150
 * 保存为：40869150
 *
 * OneNet 当前步长是 0.01，
 * 所以发送时只保留两位小数。
 *
 * 目标：
 * 40869150 -> 4087
 * 因为 4087 表示 40.87
 *
 * 为什么除以 10000：
 * x1000000 转 x100，需要缩小 10000 倍。
 *
 * 为什么加 5000：
 * 用于四舍五入。
 *
 * 例如：
 * 40.869150
 * 40869150 + 5000 = 40874150
 * 40874150 / 10000 = 4087
 * 最终显示/发送为 40.87
 */
static int32_t ESP01S_GpsX1000000_To_X100(int32_t value_x1000000)
{
    if (value_x1000000 >= 0)
    {
        return (value_x1000000 + 5000) / 10000;
    }
    else
    {
        /*
         * 负数时减 5000，也是为了实现四舍五入
         *
         * 例如：
         * -40.869150 -> -40869150
         * -40869150 - 5000 = -40874150
         * -40874150 / 10000 = -4087
         * 最终表示 -40.87
         */
        return (value_x1000000 - 5000) / 10000;
    }
}

/*
 * 把带符号的 x100 数字格式化成字符串
 *
 * x100 表示真实值放大了 100 倍。
 *
 * 例如：
 * 4087  表示 40.87
 * 11168 表示 111.68
 * -4087 表示 -40.87
 *
 * 参数：
 * out：输出字符串缓冲区
 * value_x100：放大 100 倍后的数值
 */
static void ESP01S_FormatSignedX100(char *out, int32_t value_x100)
{
    int32_t integer;
    int32_t decimal;

    /*
     * 如果是负数，先取绝对值，
     * 最后手动添加负号。
     */
    if (value_x100 < 0)
    {
        value_x100 = -value_x100;

        /*
         * 整数部分：
         * 4087 / 100 = 40
         *
         * 小数部分：
         * 4087 % 100 = 87
         */
        integer = value_x100 / 100;
        decimal = value_x100 % 100;

        /*
         * %02ld 表示小数部分固定两位，
         * 不足两位时前面补 0。
         *
         * 例如：
         * 5 会显示成 05
         */
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
 * 这个函数由 App_Task() 每 5 秒调用一次。
 *
 * 发送格式是 JSON：
 *
 * {
 *   "EnvironmentTemperature":25.0,
 *   "EnvironmentHumidity":60.0,
 *   "Latitude":40.87,
 *   "Longitude":111.68,
 *   "Altitude":0.00
 * }
 *
 * OneNet 字段：
 * EnvironmentTemperature：环境温度
 * EnvironmentHumidity：环境湿度
 * Latitude：纬度
 * Longitude：经度
 * Altitude：海拔
 *
 * 注意：
 * Latitude / Longitude / Altitude 都按两位小数发送，
 * 用来适配 OneNet 步长 0.01。
 *
 * 参数说明：
 * temperature：温度整数值，例如 25
 * humidity：湿度整数值，例如 60
 * latitude_x1000000：纬度 * 1000000
 * longitude_x1000000：经度 * 1000000
 * altitude_x100：海拔 * 100
 * gps_valid：GPS 是否定位成功，1=成功，0=未定位
 */
void ESP01S_SendData(uint8_t temperature,
                     uint8_t humidity,
                     int32_t latitude_x1000000,
                     int32_t longitude_x1000000,
                     int32_t altitude_x100,
                     uint8_t gps_valid)
{
    /*
     * buffer 用来保存最终要发送出去的 JSON 字符串
     */
    char buffer[180];

    /*
     * 保存格式化后的纬度、经度、海拔字符串
     *
     * 例如：
     * lat_str = "40.87"
     * lon_str = "111.68"
     * alt_str = "1060.50"
     */
    char lat_str[20];
    char lon_str[20];
    char alt_str[20];

    /*
     * 保存转换后的两位小数格式
     *
     * 例如：
     * lat_x100 = 4087，表示 40.87
     * lon_x100 = 11168，表示 111.68
     */
    int32_t lat_x100;
    int32_t lon_x100;

    /*
     * 如果 GPS 已定位：
     * 使用真实 GPS 坐标。
     *
     * 如果 GPS 未定位：
     * 经纬度和海拔全部发 0.00，
     * 避免上传无效坐标。
     */
    if (gps_valid)
    {
        /*
         * 经纬度从 x1000000 转成 x100，
         * 也就是从六位小数转成两位小数。
         */
        lat_x100 = ESP01S_GpsX1000000_To_X100(latitude_x1000000);
        lon_x100 = ESP01S_GpsX1000000_To_X100(longitude_x1000000);
    }
    else
    {
        lat_x100 = 0;
        lon_x100 = 0;
        altitude_x100 = 0;
    }

    /*
     * 把整数形式的数值转换成字符串
     *
     * 例如：
     * 4087  -> "40.87"
     * 11168 -> "111.68"
     * 0     -> "0.00"
     */
    ESP01S_FormatSignedX100(lat_str, lat_x100);
    ESP01S_FormatSignedX100(lon_str, lon_x100);
    ESP01S_FormatSignedX100(alt_str, altitude_x100);

    /*
     * 拼接 JSON 字符串
     *
     * 注意：
     * Latitude、Longitude、Altitude 后面使用的是 %s，
     * 但最终 JSON 里不是字符串，因为没有加双引号。
     *
     * 例如：
     * "Latitude":40.87
     *
     * 而不是：
     * "Latitude":"40.87"
     */
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

    /*
     * 通过 USART2 把 JSON 字符串发送给 ESP01S
     *
     * STM32 PA2 -> ESP01S RX
     */
    ESP01S_SendString(buffer);
}
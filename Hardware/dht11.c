#include "DHT11.h"
#include "Delay.h"

/*
 * 设置 DHT11 数据引脚为输出模式
 */
static void DHT11_Mode_Out(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * 设置 DHT11 数据引脚为输入模式
 */
static void DHT11_Mode_In(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * 输出高电平
 */
static void DHT11_DQ_High(void)
{
    GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

/*
 * 输出低电平
 */
static void DHT11_DQ_Low(void)
{
    GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

/*
 * 读取当前引脚电平
 */
static uint8_t DHT11_DQ_Read(void)
{
    return GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN);
}

/*
 * 等待引脚变为指定电平
 *
 * level = 0：等待低电平
 * level = 1：等待高电平
 *
 * 返回：
 * 0：成功
 * 1：超时
 */
static uint8_t DHT11_WaitLevel(uint8_t level, uint16_t timeout_us)
{
    while (DHT11_DQ_Read() != level)
    {
        if (timeout_us == 0)
        {
            return DHT11_ERROR;
        }

        timeout_us--;
        Delay_us(1);
    }

    return DHT11_OK;
}

/*
 * 读取 1 个字节
 */
static uint8_t DHT11_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8; i++)
    {
        /*
         * 每一位数据开始前，DHT11 会先拉低约 50us
         * 这里等待低电平结束，进入高电平阶段
         */
        DHT11_WaitLevel(1, 100);

        /*
         * 延时 40us 后判断电平
         * 如果还是高电平，说明这一位是 1
         * 如果已经变低，说明这一位是 0
         */
        Delay_us(40);

        data <<= 1;

        if (DHT11_DQ_Read() == 1)
        {
            data |= 0x01;
        }

        /*
         * 如果当前还是高电平，等待高电平结束
         * 为下一位数据做准备
         */
        DHT11_WaitLevel(0, 100);
    }

    return data;
}

/*
 * DHT11 初始化
 */
void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);

    DHT11_Mode_Out();
    DHT11_DQ_High();
}

/*
 * 读取 DHT11 温湿度
 *
 * temperature：温度整数部分
 * humidity：湿度整数部分
 *
 * 返回：
 * DHT11_OK    读取成功
 * DHT11_ERROR 读取失败
 */
uint8_t DHT11_ReadData(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t humi_int;
    uint8_t humi_dec;
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t check_sum;

    /*
     * 主机起始信号：
     * 拉低至少 18ms
     */
    DHT11_Mode_Out();

    DHT11_DQ_Low();
    Delay_ms(20);

    /*
     * 主机释放总线，拉高 20~40us
     */
    DHT11_DQ_High();
    Delay_us(30);

    /*
     * 切换为输入，等待 DHT11 响应
     */
    DHT11_Mode_In();

    /*
     * DHT11 响应：
     * 先拉低约 80us
     * 再拉高约 80us
     */
    if (DHT11_WaitLevel(0, 100) != DHT11_OK)
    {
        return DHT11_ERROR;
    }

    if (DHT11_WaitLevel(1, 100) != DHT11_OK)
    {
        return DHT11_ERROR;
    }

    if (DHT11_WaitLevel(0, 100) != DHT11_OK)
    {
        return DHT11_ERROR;
    }

    /*
     * 读取 5 字节数据
     */
    humi_int = DHT11_ReadByte();
    humi_dec = DHT11_ReadByte();
    temp_int = DHT11_ReadByte();
    temp_dec = DHT11_ReadByte();
    check_sum = DHT11_ReadByte();

    /*
     * 校验
     */
    if ((humi_int + humi_dec + temp_int + temp_dec) != check_sum)
    {
        return DHT11_ERROR;
    }

    /*
     * DHT11 一般只有整数部分
     */
    *temperature = temp_int;
    *humidity = humi_int;

    return DHT11_OK;
}
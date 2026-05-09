#include "MQ2.h"

/*
 * MQ2 初始化
 * PA1 配置为上拉输入
 */
void MQ2_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(MQ2_GPIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = MQ2_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入
    GPIO_Init(MQ2_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * 获取 MQ2 状态
 *
 * 返回：
 * MQ2_NORMAL = 正常，没有检测到烟雾/燃气
 * MQ2_ALARM  = 报警，检测到烟雾/燃气
 */
uint8_t MQ2_GetState(void)
{
    if (GPIO_ReadInputDataBit(MQ2_GPIO_PORT, MQ2_GPIO_PIN) == RESET)
    {
        return MQ2_ALARM;      // PA1 低电平，MQ2 报警
    }
    else
    {
        return MQ2_NORMAL;     // PA1 高电平，MQ2 正常
    }
}

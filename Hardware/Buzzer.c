#include "Buzzer.h"

/*
 * 蜂鸣器初始化
 *
 * PA4 配置为推挽输出
 * 高电平响，低电平不响
 */
void Buzzer_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启 GPIOA 时钟 */
    RCC_APB2PeriphClockCmd(BUZZER_GPIO_CLK, ENABLE);

    /* PA4 推挽输出 */
    GPIO_InitStructure.GPIO_Pin = BUZZER_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_GPIO_PORT, &GPIO_InitStructure);

    /* 默认关闭蜂鸣器，防止上电就响 */
    Buzzer_OFF();
}

/*
 * 打开蜂鸣器
 * 高电平触发
 */
void Buzzer_ON(void)
{
    GPIO_SetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

/*
 * 关闭蜂鸣器
 */
void Buzzer_OFF(void)
{
    GPIO_ResetBits(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN);
}

/*
 * 设置蜂鸣器状态
 *
 * state = 1：打开
 * state = 0：关闭
 */
void Buzzer_Set(uint8_t state)
{
    if (state)
    {
        Buzzer_ON();
    }
    else
    {
        Buzzer_OFF();
    }
}

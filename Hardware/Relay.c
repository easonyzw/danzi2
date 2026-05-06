#include "Relay.h"

void Relay_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * PA15 默认是 JTAG 引脚。
     * 要把 PA15 当普通 GPIO 用，需要开启 AFIO 并关闭 JTAG。
     * 这里保留 SWD 下载功能，不影响 ST-Link 下载。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    Relay1_OFF();
    Relay2_OFF();
}

void Relay1_ON(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_5);
}

void Relay1_OFF(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_5);
}

void Relay1_Set(uint8_t state)
{
    if (state)
    {
        Relay1_ON();
    }
    else
    {
        Relay1_OFF();
    }
}

void Relay2_ON(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_15);
}

void Relay2_OFF(void)
{
    GPIO_ResetBits(GPIOA, GPIO_Pin_15);
}

void Relay2_Set(uint8_t state)
{
    if (state)
    {
        Relay2_ON();
    }
    else
    {
        Relay2_OFF();
    }
}

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

void ESP01S_SendData(uint8_t temperature, uint8_t humidity)
{
    char buffer[100];

    sprintf(buffer,
            "{\"EnvironmentTemperature\":%d.0,\"EnvironmentHumidity\":%d.0}\r\n",
            temperature,
            humidity);

    ESP01S_SendString(buffer);
}
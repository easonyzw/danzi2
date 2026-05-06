#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/*
 * DHT11 数据引脚配置
 * 当前使用：PA0
 */
#define DHT11_GPIO_PORT        GPIOA
#define DHT11_GPIO_PIN         GPIO_Pin_0
#define DHT11_GPIO_CLK         RCC_APB2Periph_GPIOA

#define DHT11_OK               0
#define DHT11_ERROR            1

void DHT11_Init(void);
uint8_t DHT11_ReadData(uint8_t *temperature, uint8_t *humidity);

#endif
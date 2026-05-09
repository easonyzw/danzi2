#ifndef __MQ2_H
#define __MQ2_H

#include "stm32f10x.h"

/*
 * MQ2 烟雾/燃气传感器 DO 引脚
 * DO -> PA1
 *
 * 常见 MQ2 模块：
 * 正常无烟雾：DO = 高电平
 * 检测到烟雾/燃气：DO = 低电平
 */
#define MQ2_GPIO_PORT      GPIOA
#define MQ2_GPIO_PIN       GPIO_Pin_1
#define MQ2_GPIO_CLK       RCC_APB2Periph_GPIOA

#define MQ2_NORMAL         0
#define MQ2_ALARM          1

void MQ2_Init(void);
uint8_t MQ2_GetState(void);

#endif

#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

/*
 * 蜂鸣器引脚：PA4
 * 触发方式：高电平触发
 */
#define BUZZER_GPIO_PORT       GPIOA
#define BUZZER_GPIO_PIN        GPIO_Pin_4
#define BUZZER_GPIO_CLK        RCC_APB2Periph_GPIOA

void Buzzer_Init(void);
void Buzzer_ON(void);
void Buzzer_OFF(void);
void Buzzer_Set(uint8_t state);

#endif

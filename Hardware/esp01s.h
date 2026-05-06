#ifndef __ESP01S_H
#define __ESP01S_H

#include "stm32f10x.h"

void ESP01S_Init(void);
void ESP01S_SendByte(uint8_t byte);
void ESP01S_SendString(char *str);

void ESP01S_SendData(uint8_t temperature, uint8_t humidity);

#endif
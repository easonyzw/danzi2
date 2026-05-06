#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"

void Relay_Init(void);

void Relay1_ON(void);
void Relay1_OFF(void);
void Relay1_Set(uint8_t state);

void Relay2_ON(void);
void Relay2_OFF(void);
void Relay2_Set(uint8_t state);

#endif

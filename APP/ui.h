#ifndef __UI_H
#define __UI_H

#include "stm32f10x.h"

void UI_Init(void);
void UI_Task(uint32_t now);
void UI_ShowMainPage(void);

#endif

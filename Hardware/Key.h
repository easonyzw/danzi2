#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

/*
 * 4x4 矩阵键盘接线
 *
 * H1 -> PA6
 * H2 -> PA7
 * H3 -> PB0
 * H4 -> PB1
 *
 * L1 -> PB12
 * L2 -> PB13
 * L3 -> PB14
 * L4 -> PB15
 *
 * 返回键值：
 * 0  = 没有按键
 * 1~16 = 对应矩阵键盘 16 个按键
 */

void Key_Init(void);
uint8_t Key_GetNum(void);

#endif

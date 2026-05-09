#ifndef __APP_H
#define __APP_H

#include "stm32f10x.h"

/* 页面编号 */
#define APP_PAGE_MAIN           0       // 主界面
#define APP_PAGE_SETTING        1       // 阈值设置界面

/* 设置项编号 */
#define APP_SETTING_TEMP        0       // 温度阈值
#define APP_SETTING_HUMI        1       // 湿度阈值

void App_Init(void);
void App_Task(void);

/*
 * 给 ui.c 使用
 * 用来判断当前显示哪个界面
 */
uint8_t App_GetPage(void);

/*
 * 给 ui.c 使用
 * 用来判断阈值设置界面当前选中温度还是湿度
 */
uint8_t App_GetSettingItem(void);

#endif

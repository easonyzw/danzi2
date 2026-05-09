#include "Key.h"
#include "Delay.h"

/*
 * 行线 H1~H4：输出
 * H1 -> PA6
 * H2 -> PA7
 * H3 -> PB0
 * H4 -> PB1
 *
 * 列线 L1~L4：输入上拉
 * L1 -> PB12
 * L2 -> PB13
 * L3 -> PB14
 * L4 -> PB15
 */

/* 所有行线拉高 */
static void Key_AllRowsHigh(void)
{
    GPIO_SetBits(GPIOA, GPIO_Pin_6 | GPIO_Pin_7);
    GPIO_SetBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_1);
}

/* 设置某一行拉低，其余行拉高 */
static void Key_SetRowLow(uint8_t row)
{
    Key_AllRowsHigh();

    if (row == 1)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_6);
    }
    else if (row == 2)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
    }
    else if (row == 3)
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_0);
    }
    else if (row == 4)
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_1);
    }
}

/* 读取列线，返回第几列被按下，0表示没有按下 */
static uint8_t Key_ReadColumn(void)
{
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == RESET)
    {
        return 1;
    }

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == RESET)
    {
        return 2;
    }

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == RESET)
    {
        return 3;
    }

    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_15) == RESET)
    {
        return 4;
    }

    return 0;
}

/*
 * 扫描矩阵键盘原始键值
 *
 * 返回：
 * 0：没有按键
 * 1~16：按键编号
 *
 * 编号规则：
 * H1:  1   2   3   4
 * H2:  5   6   7   8
 * H3:  9   10  11  12
 * H4:  13  14  15  16
 */
static uint8_t Key_ScanRaw(void)
{
    uint8_t row;
    uint8_t col;
    uint8_t key_num;

    for (row = 1; row <= 4; row++)
    {
        Key_SetRowLow(row);

        /*
         * 稍微等待电平稳定
         * 软件 I/O 扫描矩阵键盘时，短延时更稳
         */
        Delay_us(5);

        col = Key_ReadColumn();

        if (col != 0)
        {
            key_num = (row - 1) * 4 + col;

            Key_AllRowsHigh();

            return key_num;
        }
    }

    Key_AllRowsHigh();

    return 0;
}

/*
 * 矩阵键盘初始化
 *
 * 行线 H1~H4 配置为推挽输出，默认高电平。
 * 列线 L1~L4 配置为上拉输入。
 */
void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * 键盘用到了 GPIOA 和 GPIOB
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /*
     * H1 -> PA6
     * H2 -> PA7
     * 配置为推挽输出
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * H3 -> PB0
     * H4 -> PB1
     * 配置为推挽输出
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*
     * L1 -> PB12
     * L2 -> PB13
     * L3 -> PB14
     * L4 -> PB15
     * 配置为上拉输入
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /*
     * 默认所有行线拉高
     */
    Key_AllRowsHigh();
}

/*
 * 获取按键编号
 *
 * 这个函数带简单消抖，并且一个按键按住时只返回一次。
 *
 * 返回：
 * 0：没有新的按键按下
 * 1~16：检测到一次新的按键按下
 */
uint8_t Key_GetNum(void)
{
    static uint8_t last_key = 0;

    uint8_t key;

    key = Key_ScanRaw();

    /*
     * 有按键按下，并且上一次是松开状态
     * 说明这是一次新的按键
     */
    if (key != 0 && last_key == 0)
    {
        Delay_ms(20);

        key = Key_ScanRaw();

        if (key != 0)
        {
            last_key = key;
            return key;
        }
    }

    /*
     * 按键松开后，允许下一次按键再次触发
     */
    if (key == 0)
    {
        last_key = 0;
    }

    return 0;
}

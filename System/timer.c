#include "Timer.h"

/*
 * 系统毫秒计数变量
 * 每进入一次 TIM2 中断，该变量加 1
 *
 * volatile 作用：
 * 告诉编译器这个变量可能会在中断中被修改，
 * 不要对它进行错误优化。
 */
static volatile uint32_t g_ms_count = 0;

/*
 * Timer_Init
 * 功能：初始化 TIM2 定时器，使其每 1ms 产生一次中断
 */
void Timer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /*
     * 开启 TIM2 外设时钟
     * TIM2 挂载在 APB1 总线上
     */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /*
     * 定时器计数频率计算：
     *
     * STM32F103 常见主频为 72MHz
     * TIM2 定时器时钟一般也是 72MHz
     *
     * 72MHz / 7200 = 10000Hz
     * 10000Hz 表示计数器每 0.1ms 加 1
     *
     * 自动重装载值设置为 10 - 1
     * 计数 10 次产生一次更新中断
     *
     * 0.1ms * 10 = 1ms
     *
     * 所以 TIM2 每 1ms 进入一次中断
     */
    TIM_TimeBaseStructure.TIM_Period = 10 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 7200 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /*
     * 使能 TIM2 更新中断
     * 当定时器计数溢出时，会触发 TIM2_IRQHandler
     */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /*
     * 配置 NVIC 中断控制器
     * 允许 TIM2 中断进入 CPU
     */
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /*
     * 启动 TIM2 定时器
     * 从这里开始，TIM2 每 1ms 产生一次中断
     */
    TIM_Cmd(TIM2, ENABLE);
}

/*
 * Timer_GetMs
 * 功能：获取系统已经运行的毫秒数
 *
 * 返回值：
 * g_ms_count：系统运行时间，单位 ms
 */
uint32_t Timer_GetMs(void)
{
    return g_ms_count;
}

/*
 * TIM2 中断服务函数
 * TIM2 每 1ms 进入一次该函数
 */
void TIM2_IRQHandler(void)
{
    /*
     * 判断是否为 TIM2 更新中断
     */
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
    {
        /*
         * 毫秒计数加 1
         * 每加 1 表示系统运行时间增加 1ms
         */
        g_ms_count++;

        /*
         * 清除 TIM2 更新中断标志位
         * 如果不清除，会一直重复进入中断
         */
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
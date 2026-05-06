#include "stm32f10x.h"
#include "hardware.h"
#include "App.h"

int main(void)
{
    Hardware_Init();
    App_Init();

    while (1)
    {
        App_Task();
    }
}
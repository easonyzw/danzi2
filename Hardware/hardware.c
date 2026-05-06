#include "hardware.h"
#include "Timer.h"
#include "ESP01S.h"
#include "dht11.h"
#include "Buzzer.h"

void Hardware_Init(void)
{
    Timer_Init();
    ESP01S_Init();
    DHT11_Init();
    Buzzer_Init();
}

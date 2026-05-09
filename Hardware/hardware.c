#include "hardware.h"
#include "Timer.h"
#include "ESP01S.h"
#include "dht11.h"
#include "Buzzer.h"
#include "Relay.h"
#include "gps.h"
#include "OLED.h"
#include "Key.h"
#include "MQ2.h"
void Hardware_Init(void)
{
    Timer_Init();
    ESP01S_Init();
    DHT11_Init();
    Buzzer_Init();
    Relay_Init();
    OLED_Init();
    Key_Init();
    GPS_Init();      // 默认 9600 波特率
}

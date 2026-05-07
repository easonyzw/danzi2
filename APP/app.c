#include "App.h"
#include "Data.h"
#include "ESP01S.h"
#include "Timer.h"
#include "DHT11.h"
#include "gps.h"
#include "ui.h"

static uint32_t last_dht_time = 0;
static uint32_t last_esp_time = 0;

void App_Init(void)
{
    UI_Init();
}

void App_Task(void)
{
    uint32_t now = Timer_GetMs();

    GPS_Task();

    if (now - last_dht_time >= 2000)
    {
        last_dht_time = now;

        if (DHT11_ReadData(&g_sensor.temperature, &g_sensor.humidity) == DHT11_OK)
        {
        }
        else
        {
        }
    }

    UI_Task(now);

    if (now - last_esp_time >= 5000)
    {
        last_esp_time = now;

        ESP01S_SendData(g_sensor.temperature,
                        g_sensor.humidity,
                        GPS_Data.latitude_x1000000,
                        GPS_Data.longitude_x1000000,
                        GPS_Data.altitude_x100,
                        GPS_Data.valid);
    }
}
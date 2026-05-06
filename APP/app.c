#include "App.h"
#include "Data.h"
#include "ESP01S.h"
#include "Timer.h"
#include "DHT11.h"

static uint32_t last_dht_time = 0;
static uint32_t last_esp_time = 0;

void App_Init(void)
{

}

void App_Task(void)
{
    uint32_t now = Timer_GetMs();

    /*
     * 每 2 秒读取一次 DHT11
     * DHT11 不建议读取太频繁
     */
    if (now - last_dht_time >= 2000)
    {
        last_dht_time = now;

        if (DHT11_ReadData(&g_sensor.temperature, &g_sensor.humidity) == DHT11_OK)
        {
            /*
             * 读取成功，g_sensor.temperature 和 g_sensor.humidity 已经更新
             */
        }
        else
        {
            /*
             * 读取失败，保持上一次数据不变
             */
        }
    }

    /*
     * 每 5 秒发送一次温湿度给 ESP
     */
    if (now - last_esp_time >= 5000)
    {
        last_esp_time = now;

        ESP01S_SendData(g_sensor.temperature,
                        g_sensor.humidity);
    }
}
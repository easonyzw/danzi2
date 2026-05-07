#include "App.h"
#include "Data.h"
#include "ESP01S.h"
#include "Timer.h"
#include "DHT11.h"
#include "gps.h"

static uint32_t last_dht_time = 0;
static uint32_t last_esp_time = 0;

void App_Init(void)
{

}

void App_Task(void)
{
    uint32_t now = Timer_GetMs();

    /*
     * GPS 串口数据是中断接收的。
     * 这里循环调用 GPS_Task()，有新数据时会自动解析。
     */
    GPS_Task();

    /*
     * 每 2 秒读取一次 DHT11
     */
    if (now - last_dht_time >= 2000)
    {
        last_dht_time = now;

        if (DHT11_ReadData(&g_sensor.temperature, &g_sensor.humidity) == DHT11_OK)
        {
            /*
             * 读取成功
             */
        }
        else
        {
            /*
             * 读取失败，保持上一次数据
             */
        }
    }

    /*
     * 每 5 秒发送一次温湿度 + GPS 坐标给 ESP
     */
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

#ifndef __DATA_H
#define __DATA_H

#include "stm32f10x.h"

typedef struct
{
    uint8_t temperature;
    uint8_t humidity;
} SensorData_t;

extern SensorData_t g_sensor;

#endif
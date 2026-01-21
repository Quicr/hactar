#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include "stm32f072xb.h"
#include "stm32f0xx_hal_adc.h"

enum PowerMode
{
    Usb = 0,
    Battery,
};

typedef struct
{
    uint8_t usb_connected;
    uint8_t charging;
    uint8_t battery_level;
} power_info_t;

void power_sensor_detect_usb(power_info_t* info);
void power_sensor_detect_charging(power_info_t* info);
void power_sensor_measure_charge(power_info_t* info);

#endif

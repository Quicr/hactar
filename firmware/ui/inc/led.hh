#pragma once

#include "stm32.h"
#include "stm32f4xx_hal_gpio.h"
#include <cstdint>

enum class LED_Polarity
{
    Low = 0, // Active low, e.g. low turns LED on
    High     // Active high
};

class LED
{
public:
    LED(GPIO_TypeDef* port, int pin, LED_Polarity polarity);
    void Off();
    void On();

    uint32_t BSSROnValue();
    uint32_t BSSROffValue();

    uint32_t* BSSRAddr();

private:
    GPIO_TypeDef* port;
    int pin;
    LED_Polarity polarity;
};

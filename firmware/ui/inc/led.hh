#pragma once

#include "stm32.h"
#include "stm32f4xx_hal_gpio.h"

enum class LED_Polarity
{
    Low, // Active low, e.g. low turns LED on
    High // Active high
};

class LED
{
private:
    enum class Mode
    {
        Off,
        On,
        Breathe,
        Flash,
        Set,
    };

public:
    LED(GPIO_TypeDef* port, int pin, LED_Polarity polarity);
    void On();
    void Off();
    void Breathe();
    void Flash();
    void Set(int pwm_value);
    static void Callback(LED& led);

private:
    GPIO_TypeDef* port;
    int pin;
    LED_Polarity polarity;
    Mode mode;
    int pwm_value;
};

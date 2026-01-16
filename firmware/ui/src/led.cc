#include "led.hh"
#include "stm32f4xx_hal_gpio.h"

LED::LED(GPIO_TypeDef* port, int pin, LED_Polarity polarity) :
    port(port),
    pin(pin),
    polarity(polarity)
{
    Off();
}

void LED::Off()
{
    port->BSRR |= BSSROffValue();
}

void LED::On()
{
    port->BSRR |= BSSROnValue();
}

uint32_t LED::BSSROnValue()
{
    return pin << ((1 - (int)polarity) * 16);
}

uint32_t LED::BSSROffValue()
{
    return pin << ((int)polarity * 16);
}

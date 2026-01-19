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
    port->BSRR |= BSRROffValue();
}

void LED::On()
{
    port->BSRR |= BSRROnValue();
}

uint32_t LED::BSRROnValue()
{
    return pin << ((1 - (int)polarity) * 16);
}

uint32_t LED::BSRROffValue()
{
    return pin << ((int)polarity * 16);
}

volatile uint32_t* LED::BSRRAddr()
{
    return &port->BSRR;
}

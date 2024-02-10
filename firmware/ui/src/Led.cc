#include "Led.hh"

Led::Led(GPIO_TypeDef* port,
    uint32_t pin,
    uint16_t _on_value,
    uint16_t _off_value,
    uint32_t duration) :
    port(port),
    pin(pin),
    on_value(_on_value),
    off_value(_off_value),
    duration(duration),
    is_on(false),
    timeout(0)
{

}

Led::~Led()
{

}

void Led::On()
{
    timeout = HAL_GetTick() + duration;
    is_on = true;
    HAL_GPIO_WritePin(port,
        pin,
        (GPIO_PinState)on_value);
}

void Led::Off()
{
    is_on = false;
    HAL_GPIO_WritePin(port,
        pin,
        (GPIO_PinState)off_value);
}

void Led::Toggle()
{
    if (is_on)
    {
        Off();
    }
    else
    {
        On();
    }
}

void Led::Timeout()
{
    if (!is_on)
        return;

    if (HAL_GetTick() < timeout)
        return;

    Off();
}

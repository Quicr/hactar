#include "Led.hh"

Led::Led(GPIO_TypeDef* port, uint32_t pin, uint32_t duration) :
    group({port, pin}),
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
    HAL_GPIO_WritePin(group.port, group.pin, GPIO_PIN_SET);
}

void Led::Off()
{
    is_on = false;
    HAL_GPIO_WritePin(group.port, group.pin, GPIO_PIN_RESET);
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

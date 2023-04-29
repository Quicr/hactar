#pragma once

#include "stm32.h"
#include "PortPin.hh"

class Led
{
public:
    Led(GPIO_TypeDef* port, uint32_t pin, uint32_t duration = 1000);
    ~Led();

    void On();
    void Off();
    void Toggle();
    void Timeout();

private:
    port_pin group;
    uint32_t duration;
    bool is_on;
    uint32_t timeout;
};
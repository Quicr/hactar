#pragma once

#include "stm32.h"
#include "port_pin.hh"

class Led
{
public:
    Led(GPIO_TypeDef* port,
        uint32_t pin,
        uint16_t _on_value,
        uint16_t _off_value,
        uint32_t duration = 1000);
    ~Led();

    void On();
    void Off();
    void Toggle();
    void Timeout();

private:
    GPIO_TypeDef* port;
    uint32_t pin;
    uint16_t on_value;
    uint16_t off_value;
    uint32_t duration;
    bool is_on;
    uint32_t timeout;
};
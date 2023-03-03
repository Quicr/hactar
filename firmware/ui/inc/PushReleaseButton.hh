#pragma once
#include "stm32.h"
#include "PortPin.hh"

class PushReleaseButton
{
public:
    PushReleaseButton(port_pin &group,
                      uint16_t debounce_duration);

    void Begin();
    bool Read();
private:
    port_pin &group;

    uint16_t debounce_duration;
    uint16_t btn_latch;
    uint16_t btn_released;
    uint16_t btn_read;
    uint32_t repeat_at;
};
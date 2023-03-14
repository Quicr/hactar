#include "PushReleaseButton.hh"
#include "Helper.h"

PushReleaseButton::PushReleaseButton(port_pin &group,
                                     uint16_t debounce_duration) :
    group(group),
    debounce_duration(debounce_duration),
    btn_latch(0),
    btn_released(0),
    btn_read(0),
    repeat_at(0)
{
}

void PushReleaseButton::Begin()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    EnablePortIf(group.port);

    GPIO_InitStruct.Pin = group.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(group.port, &GPIO_InitStruct);
}

bool PushReleaseButton::Read()
{
    uint16_t btn_down = HAL_GPIO_ReadPin(group.port, group.pin);
    if (btn_down)
    {
        if (!btn_latch) repeat_at = HAL_GetTick() + debounce_duration;
        btn_latch = btn_down;
    }

    if (btn_down != btn_latch)
    {
        btn_latch = 0;
        repeat_at = 0;

        return true;
    }

    if (HAL_GetTick() > repeat_at && btn_latch)
    {
        repeat_at += 50;
        return true;
    }

    return false;
}
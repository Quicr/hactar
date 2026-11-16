#pragma once

#include "stdint.h"
#include "stm32.h"

struct Font
{
    const uint8_t width;
    const uint8_t height;
    const uint8_t* data;
};

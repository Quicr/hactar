#pragma once

#include "stdint.h"

struct Font
{
    const uint8_t width;
    const uint8_t height;
    const uint8_t* data;
};

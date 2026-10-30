#pragma once

#include <cstddef>
#include <cstdint>

struct Bitmap
{
    const size_t width;
    const size_t height;

    const uint8_t* data;
};

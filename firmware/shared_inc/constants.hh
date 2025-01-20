#pragma once
#include <stdint.h>

namespace constants
{
    static constexpr uint16_t Sample_Rate = 8'000; // 8khz

    static constexpr uint16_t Audio_Buffer_Sz = 640;
    static constexpr uint16_t Audio_Buffer_Sz_2 = Audio_Buffer_Sz / 2;
}
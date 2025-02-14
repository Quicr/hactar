#pragma once
#include <stdint.h>

namespace constants
{
    static constexpr uint16_t Sample_Rate = 16'000;
    static constexpr float Bytes_Per_Sample = 0.08; // Calculated amount, no touchy
    static constexpr uint16_t Audio_Buffer_Sz = Sample_Rate * Bytes_Per_Sample;
    static constexpr uint16_t Audio_Buffer_Sz_2 = Audio_Buffer_Sz / 2;
}
#pragma once
#include <stdint.h>

namespace constants
{
    static constexpr uint16_t Audio_Time_Length_ms = 20;
    static constexpr float Audio_Time_Length_s = Audio_Time_Length_ms / 1000.0;
    static constexpr uint16_t Sample_Rate = 8'000;
    static constexpr uint16_t Num_Channels = 2;
    static constexpr uint16_t Num_Buffers = 2; // double buff
    static constexpr uint16_t Audio_Buffer_Sz = Num_Buffers * Sample_Rate * Num_Channels * Audio_Time_Length_s;
    static constexpr uint16_t Audio_Buffer_Sz_2 = Audio_Buffer_Sz / 2;
}
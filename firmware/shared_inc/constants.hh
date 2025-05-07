#pragma once
#include <stdint.h>

namespace constants
{
enum class SampleRates
{
    _8khz = 8'000,
    _16khz = 16'000
};

static constexpr uint16_t Audio_Time_Length_ms = 20;
static constexpr float Audio_Time_Length_s = Audio_Time_Length_ms / 1000.0;
static constexpr SampleRates Sample_Rate = SampleRates::_8khz;
static constexpr uint16_t Stereo = 0;
static constexpr uint16_t Num_Buffers = 2; // double buff
// There are always two channels coming from the wm8960, we have to do some processing ourself.
static constexpr uint16_t Total_Audio_Buffer_Sz =
    Num_Buffers * (uint16_t)Sample_Rate * 2 * Audio_Time_Length_s;
static constexpr uint16_t Audio_Buffer_Sz = Total_Audio_Buffer_Sz / 2;
// Note- Expanded and Companded have the same amount of elements, but half the bytes
static constexpr uint16_t Audio_Phonic_Sz = Stereo > 0 ? Audio_Buffer_Sz : Audio_Buffer_Sz / 2;
} // namespace constants
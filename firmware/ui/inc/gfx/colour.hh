#pragma once

#include <stdint-gcc.h>

enum class Colour : uint8_t
{
    Black,
    Grey,
    White,
    Red,
    Yellow,
    Light_Green,
    Green,
    Cyan,
    Blue,
    Magenta,
};

enum Colour16 : uint16_t
{
    Black = 0x0001U,
    Grey = 0xCE59U,
    White = 0xFFFFU,
    Red = 0xF800U,
    Yellow = 0xFFE0U,
    Light_Green = 0x3626U,
    Green = 0x07E0U,
    Cyan = 0x07FFU,
    Blue = 0x001FU,
    Magenta = 0xF81FU,
};

static constexpr uint16_t Colour16_Map[]{
    static_cast<uint16_t>(Colour16::Black),  static_cast<uint16_t>(Colour16::Grey),
    static_cast<uint16_t>(Colour16::White),  static_cast<uint16_t>(Colour16::Red),
    static_cast<uint16_t>(Colour16::Yellow), static_cast<uint16_t>(Colour16::Light_Green),
    static_cast<uint16_t>(Colour16::Green),  static_cast<uint16_t>(Colour16::Cyan),
    static_cast<uint16_t>(Colour16::Blue),   static_cast<uint16_t>(Colour16::Magenta),
};

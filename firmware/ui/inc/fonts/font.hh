#pragma once

#include "stm32.h"
#include "stdint.h"

enum class Fonts {
    _5x8,
    _6x8,
    _7x12,
    _11x16
};

typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint8_t *data;
} Font;

extern Font font5x8;
extern Font font6x8;
extern Font font7x12;
extern Font font11x16;
extern Font mic5x8;


static Font& GetFont(Fonts font) __attribute__((unused));
static Font& GetFont(Fonts font)
{
    switch (font)
    {
        case Fonts::_5x8:
        {
            return font5x8;
            break;
        }
        case Fonts::_6x8:
        {
            return font6x8;
            break;
        }
        case Fonts::_7x12:
        {
            return font7x12;
            break;
        }
        case Fonts::_11x16:
        {
            return font11x16;
            break;
        }
        default:
        {
            break;
        }
    }

    // Just return a font
    return font5x8;
}
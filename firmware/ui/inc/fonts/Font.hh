#pragma once

#include "stm32.h"
#include "stdint.h"

typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint8_t *data;
} Font;

extern Font font5x8;
extern Font font6x8;
extern Font font7x12;
extern Font font11x16;

#pragma once

#include "font.hh"

// typedef struct
// {
//     const unsigned char width;
//     const unsigned char height;
//     const unsigned char* data;
// } Symbol;

const unsigned char mic5x8_table[] = {
    0x70,   // 01110000
    0xF8,   // 11111000
    0xF8,   // 11111000
    0xF8,   // 11111000
    0x70,   // 01110000
    0x00,   // 10001000
    0x20,   // 00100000
    0x20    // 00100000
};

Font mic5x8 = {
    5,
    8,
    mic5x8_table
};
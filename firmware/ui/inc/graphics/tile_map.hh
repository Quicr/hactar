#pragma once

#include "graphics.hh"
#include <ctype.h>
#include <cstdint>

class TileMap
{
public:
    TileMap(uint8_t* tiles,
            const size_t size,
            const size_t width,
            const size_t height,
            const size_t tile_size);

    void Mark(Graphics::bound_t rect);
    void Mark(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void GetNext(Graphics::bound_t& rect);

private:
    const uint8_t* tiles;
    const size_t size;
    const size_t width;
    const size_t height;
    const size_t tile_size;
    const uint16_t cols;
    const uint16_t rows;
};

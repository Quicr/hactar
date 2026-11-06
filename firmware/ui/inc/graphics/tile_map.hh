#pragma once

#include "graphics.hh"
#include <ctype.h>
#include <algorithm>
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
    Graphics::bound_t GetNext();
    void FlipOrientation();
    bool HasDirty() const;

    static constexpr size_t GetSize(const size_t width, const size_t height, const size_t tile_size)
    {
        return std::max<size_t>(1, ((GetDimension(width, tile_size) + ((sizeof(*tiles) * 8) - 1))
                                    / (sizeof(*tiles) * 8))
                                       * GetDimension(height, tile_size));
    }

private:
    uint8_t TakeTile(const uint16_t col, const uint16_t row) const;
    uint8_t ViewTile(const uint16_t col, const uint16_t row) const;
    void ClearTile(const uint16_t col, const uint16_t row) const;
    void ClearTileRow(const uint16_t col1, const uint16_t col2, const uint16_t row) const;
    inline uint8_t GetTileMask(const uint16_t col) const;
    inline uint16_t GetTileCol(const uint16_t col) const;
    inline uint16_t GetTileRow(const uint16_t row) const;
    inline uint16_t GetAddr(const uint16_t col, const uint16_t row) const;

    static constexpr size_t GetDimension(const size_t dimension, const size_t tile_size)
    {
        return (dimension + (tile_size - 1)) / tile_size;
    }

    uint8_t* const tiles;
    const size_t size;
    size_t width;
    size_t height;
    const size_t tile_size;
    const uint16_t cols;
    const uint16_t rows;
    bool has_dirty;
};

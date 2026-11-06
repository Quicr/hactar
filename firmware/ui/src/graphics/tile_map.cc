#include "tile_map.hh"
#include "app_main.hh"
#include "logger.hh"
#include <algorithm>
#include <cassert>
#include <cstdint>

namespace
{
uint8_t* CheckTiles(uint8_t* const tiles)
{
    if (tiles == 0)
    {

        Error("TileMap constructor : tiles", "Tiles is nullptr");
        return nullptr;
    }

    return tiles;
}
size_t CheckSize(const size_t size)
{
    if (size == 0)
    {
        Error("TileMap constructor : size", "Invalid size of zero");
        return 1;
    }

    return size;
}
size_t CheckTileSize(const size_t size)
{
    if (size == 0)
    {
        Error("TileMap constructor : tile_size", "Invalid tile_size of zero");
        return 1;
    }

    return size;
}
} // namespace

TileMap::TileMap(uint8_t* const tiles,
                 const size_t size,
                 const size_t width,
                 const size_t height,
                 const size_t tile_size) :
    tiles(CheckTiles(tiles)),
    size(CheckSize(size)),
    width(width),
    height(height),
    tile_size(CheckTileSize(tile_size)),
    cols(GetDimension(width, this->tile_size)),
    rows(GetDimension(height, this->tile_size)),
    has_dirty(false)
{
    assert(this->size > 0);
    assert(this->tiles != nullptr);
    memset(this->tiles, 0, sizeof(*this->tiles) * this->size);

    assert(GetSize(this->width, this->height, this->tile_size) == this->size);
}

void TileMap::Mark(Graphics::bound_t rect)
{
    Mark(rect.x1, rect.y1, rect.x2, rect.y2);
}

void TileMap::Mark(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // Clamp inside of bounds
    x1 = std::min<uint16_t>(x1, width);
    y1 = std::min<uint16_t>(y1, height);

    x2 = std::min<uint16_t>(x2, width);
    y2 = std::min<uint16_t>(y2, height);

    if (x1 >= x2 || y1 >= y2)
    {
        return;
    }

    // Clamp to nearest tile
    x1 /= tile_size;
    x2 = (x2 + tile_size - 1) / tile_size;

    y1 /= tile_size;
    y2 = (y2 + tile_size - 1) / tile_size;

    for (uint16_t y = y1; y < y2; ++y)
    {
        const uint16_t tile_row = GetTileRow(y);
        for (uint16_t x = x1; x < x2; ++x)
        {
            const uint16_t tile_col = GetTileCol(x);
            const uint16_t mask = GetTileMask(x);

            tiles[tile_col + tile_row] |= mask;
        }
    }
    has_dirty = true;
}

Graphics::bound_t TileMap::GetNext()
{
    Graphics::bound_t rect = {
        .x1 = 0,
        .y1 = 0,
        .x2 = 0,
        .y2 = 0,
    };

    if (!has_dirty)
    {
        return rect;
    }

    for (uint16_t row = 0; row < rows; ++row)
    {
        for (uint16_t col = 0; col < cols; ++col)
        {
            if (!TakeTile(col, row))
            {
                continue;
            }

            // found a dirty tile
            rect.x1 = col * tile_size;
            rect.y1 = row * tile_size;

            // Greedy adjecent tile approach
            for (rect.x2 = col + 1; rect.x2 < cols; ++rect.x2)
            {
                if (!TakeTile(rect.x2, row))
                {
                    break;
                }
            }

            for (rect.y2 = row + 1; rect.y2 < rows; ++rect.y2)
            {
                bool row_dirty = true;
                for (uint16_t adj_col = col; adj_col < rect.x2; ++adj_col)
                {
                    if (!ViewTile(adj_col, rect.y2))
                    {
                        row_dirty = false;
                        break;
                    }
                }

                if (!row_dirty)
                {
                    break;
                }

                // Clear tile row
                ClearTileRow(col, rect.x2, rect.y2);
            }

            rect.x2 = std::min(rect.x2 * tile_size, width);
            rect.y2 = std::min(rect.y2 * tile_size, height);
            return rect;
        }
    }

    has_dirty = false;
    return rect;
}

void TileMap::MarkAll()
{
    has_dirty = true;
    for (size_t i = 0; i < size; ++i)
    {
        tiles[i] = 0xFF;
    }
}

void TileMap::FlipOrientation()
{
    // TODO rotate any tiles
    const uint16_t tmp = width;

    width = height;
    height = tmp;

    UI_LOG_ERROR("TileMap::FlipOrientation is not implemented!");
}

bool TileMap::HasDirty() const
{
    return has_dirty;
}

uint8_t TileMap::TakeTile(const uint16_t col, const uint16_t row) const
{
    const uint16_t addr = GetAddr(col, row);
    const uint8_t mask = GetTileMask(col);

    const uint8_t val = tiles[addr] & mask;
    tiles[addr] &= ~mask;
    return val;
}

uint8_t TileMap::ViewTile(const uint16_t col, const uint16_t row) const
{
    const uint16_t addr = GetAddr(col, row);
    const uint8_t mask = GetTileMask(col);

    return tiles[addr] & mask;
}

void TileMap::ClearTile(const uint16_t col, const uint16_t row) const
{
    const uint16_t addr = GetAddr(col, row);
    const uint8_t mask = GetTileMask(col);

    tiles[addr] &= ~mask;
}

void TileMap::ClearTileRow(const uint16_t col1, const uint16_t col2, const uint16_t row) const
{
    for (uint16_t col = col1; col < col2; ++col)
    {
        ClearTile(col, row);
    }
}

uint8_t TileMap::GetTileMask(const uint16_t col) const
{
    const uint16_t tile_bit = col % (sizeof(*tiles) * 8);
    const uint16_t mask = 1 << (((sizeof(*tiles) * 8) - 1) - tile_bit);
    return mask;
}

inline uint16_t TileMap::GetAddr(const uint16_t col, const uint16_t row) const
{
    return GetTileCol(col) + GetTileRow(row);
}

inline uint16_t TileMap::GetTileCol(const uint16_t col) const
{
    return col / (sizeof(*tiles) * 8);
}

inline uint16_t TileMap::GetTileRow(const uint16_t row) const
{
    return row * ((cols + ((sizeof(*tiles) * 8) - 1)) / (sizeof(*tiles) * 8));
}

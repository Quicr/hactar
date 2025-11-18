#pragma once

#include "gfx/colour.hh"
#include "gfx/graphic_memory.hh"
#include <stdint-gcc.h>
#include <span>

class Polygon
{
public:
    typedef void (*DrawCallback)(GraphicMemory* memory,
                                 uint8_t* matrix,
                                 const uint16_t m_width,
                                 const uint16_t scan_line_y0,
                                 const uint16_t scan_line_y1);

    static constexpr size_t Num_Bytes_Size = sizeof(uint8_t);
    static constexpr size_t DrawCallbackSize = sizeof(DrawCallback);

    enum class Type : uint8_t
    {
        Empty = 0,
        SolidRectangle,

    };
    // virtual void Serialize(std::span<uint8_t>& span);
    // virtual size_t Size();

    // virtual bool
    // Draw(uint8_t* matrix, const uint16_t m_width, const uint16_t y0, const uint16_t y1);

    static void FillPixel(uint8_t* matrix,
                          const uint16_t m_width,
                          const uint16_t x,
                          const uint16_t y,
                          const uint8_t colour_high,
                          const uint8_t colour_low)
    {
        // Each byte stores two pixels of colour.
        // The first half is the "even" pixel and the second half is the "odd" pixel
        // e.g.
        // 0 1 2 3 (pixels 0 and 2 are "even" pixels in idx)

        const uint16_t idx = x / 2;
        if (x & 0x0001)
        {
            matrix[idx + m_width * y] = (matrix[idx + m_width * y] & 0xF0) | colour_low;
        }
        else
        {
            matrix[idx + m_width * y] = (matrix[idx + m_width * y] & 0x0F) | colour_high;
        }
    }

private:
};
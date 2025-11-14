#pragma once

#include "gfx/colour.hh"
#include <stdint-gcc.h>

class Shape
{
public:
    Shape()
    {
    }

    virtual bool Draw(uint8_t* matrix, const uint16_t m_width, const uint16_t y0, const uint16_t y1)
    {
        return true;
    };

    void FillPixel(uint8_t* matrix,
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

protected:
    virtual bool InScanLineBound(const uint16_t scan_line_y0, const uint16_t scan_line_y1)
    {
        return false;
    };
    virtual bool OutOfScanLineBound(const uint16_t scan_line_y0, const uint16_t scan_line_y1)
    {
        return true;
    };

private:
};
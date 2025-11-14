#pragma once

#include "gfx/colour.hh"
#include "shape.hh"
#include <stdint-gcc.h>

class SolidRectangle : public Shape
{
public:
    bool Draw(uint8_t* matrix,
              const uint16_t m_width,
              const uint16_t scan_line_y0,
              const uint16_t scan_line_y1) override
    {
        if (OutOfScanLineBound(scan_line_y0, scan_line_y1))
        {
            return false;
        }

        const uint8_t colour_high = static_cast<const uint8_t>(colour) << 4;
        const uint8_t colour_low = static_cast<const uint8_t>(colour) & 0x0F;

        uint16_t current_x = x;
        uint16_t current_y = y > scan_line_y0 ? y : scan_line_y0;

        const uint16_t end_x = current_x + width < m_width ? current_x + width : m_width;
        const uint16_t end_y =
            current_y + height < scan_line_y1 ? current_y + height : scan_line_y1;

        for (current_y; current_y < end_y; ++current_y)
        {
            for (current_x; current_x < end_x; ++current_x)
            {
                FillPixel(matrix, m_width, current_x, current_y, colour_high, colour_low);
            }
        }

        return end_y == scan_line_y1;
    }

protected:
    inline bool InScanLineBound(const uint16_t scan_line_y0, const uint16_t scan_line_y1) override
    {
        return scan_line_y0 < y + width && scan_line_y1 > y;
    }

    inline bool OutOfScanLineBound(const uint16_t scan_line_y0,
                                   const uint16_t scan_line_y1) override
    {
        return scan_line_y0 >= y + width || scan_line_y1 <= y;
    }

    bool done;
    const uint16_t x;
    const uint16_t y;
    const uint16_t width;
    const uint16_t height;
    const Colour colour;
};
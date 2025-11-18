#pragma once

#include "gfx/colour.hh"
#include "gfx/embedded_display.hh"
#include "gfx/graphic_memory.hh"
#include "gfx/shapes/polygon.hh"
#include <ring_buffer.hh>
#include <stdint-gcc.h>
#include <cstddef>
#include <span>

class __attribute__((packed)) SolidRectangle : public Polygon
{
public:
    // SolidRectangle = delete;
    // SolidRectangle(const uint16_t x,
    //                const uint16_t y,
    //                const uint16_t width,
    //                const uint16_t height,
    //                const uint8_t colour) :
    //     x(x),
    //     y(y),
    //     width(width),
    //     height(height),
    //     colour(colour),
    //     done(false)
    // {
    // }

    static void Draw(GraphicMemory& memory,
                     uint16_t x,
                     uint16_t y,
                     const uint16_t width,
                     const uint16_t height,
                     Colour colour)
    {
        static constexpr size_t Total_Size = DrawCallbackSize + sizeof(SolidRectangle);

        std::span<uint8_t> buff = memory.AllocateMemory(Total_Size);

        memory.PushParameter(buff, Total_Size);
        memory.PushParameter(buff, reinterpret_cast<std::uintptr_t>(&Render));
        memory.PushParameter(buff, static_cast<const uint8_t>(Polygon::Type::SolidRectangle));
        memory.PushParameter(buff, x);
        memory.PushParameter(buff, y);
        memory.PushParameter(buff, width);
        memory.PushParameter(buff, height);
        memory.PushParameter(buff, static_cast<uint8_t>(colour));
    }

    static bool Render(std::span<uint8_t> buff,
                       uint8_t* matrix,
                       const uint16_t m_width,
                       const uint16_t scan_line_y0,
                       const uint16_t scan_line_y1)
    {
        // Static cast the buff to our type

        // Pull data out of as much memory as we need.
        // auto buff =
        //     memory.Retrieve

        // if (OutOfScanLineBound(*rectangle, scan_line_y0, scan_line_y1))
        // {
        //     return false;
        // }

        // const uint8_t colour_high = static_cast<const uint8_t>(rectangle->colour) << 4;
        // const uint8_t colour_low = static_cast<const uint8_t>(rectangle->colour) & 0x0F;

        // uint16_t current_x = rectangle->x;
        // uint16_t current_y = rectangle->y > scan_line_y0 ? rectangle->y : scan_line_y0;

        // const uint16_t end_x =
        //     current_x + rectangle->width < m_width ? current_x + rectangle->width : m_width;
        // const uint16_t end_y = current_y + rectangle->height < scan_line_y1
        //                          ? current_y + rectangle->height
        //                          : scan_line_y1;

        // for (current_y; current_y < end_y; ++current_y)
        // {
        //     for (current_x; current_x < end_x; ++current_x)
        //     {
        //         FillPixel(matrix, m_width, current_x, current_y, colour_high, colour_low);
        //     }
        // }

        // rectangle->done = rectangle->y + rectangle->height >= scan_line_y1;

        return true;
    }

private:
    static bool InScanLineBound(const SolidRectangle& rectangle,
                                const uint16_t scan_line_y0,
                                const uint16_t scan_line_y1)
    {
        return scan_line_y0 < rectangle.y + rectangle.width && scan_line_y1 > rectangle.y;
    }

    static bool OutOfScanLineBound(const SolidRectangle& rectangle,
                                   const uint16_t scan_line_y0,
                                   const uint16_t scan_line_y1)
    {
        return scan_line_y0 >= rectangle.y + rectangle.width || scan_line_y1 <= rectangle.y;
    }

    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t colour;
    uint8_t done;
};

static_assert(sizeof(SolidRectangle) == 10U);
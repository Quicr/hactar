#pragma once

#include "font.hh"
#include <cstdint>

class Graphics
{
public:
    enum class Colour : uint8_t
    {
        Black = 0,
        White,
        Blue,
        Red,
        Light_Green,
        Green,
        Cyan,
        Magenta,
        Yellow,
        Grey,
    };

    enum class ShapeType
    {
        Pixel,
        Line,
        Rectangle,
        Circle,
        String,
        Bitmap
    };

    struct BitmapData
    {
        // Note, that our colours are 4 bit quantized, so width is acutally 32
        static constexpr size_t width = 16;
        static constexpr size_t height = 32;

        uint8_t data[height][width];
    };

    struct Shape
    {
        enum Flags : uint8_t
        {
            Done = 1 << 0,
            Filled = 1 << 1,
        };

        ShapeType type;

        union
        {
            struct
            {
                uint16_t x;
                uint16_t y;
                Colour colour;
            } pixel;

            struct
            {
                uint16_t x1;
                uint16_t y1;
                uint16_t x2;
                uint16_t y2;
                Colour colour;
                uint8_t flags;
            } line;

            struct
            {
                uint16_t x1;
                uint16_t y1;
                uint16_t x2;
                uint16_t y2;
                Colour colour;
                uint8_t flags;
            } rectangle;

            struct
            {
                uint16_t x;
                uint16_t y;
                uint16_t r;
                Colour colour;
                uint8_t flags;
            } circle;

            struct
            {
                uint16_t x;
                uint16_t y;
                Font& font;
                Colour foreground;
                Colour background;
                uint8_t flags;
            } string;

            struct
            {
                uint16_t x;
                uint16_t y;
                BitmapData* map;
            } bitmap;
        };
    };

    bool Rasterize(Shape& shape,
                   uint8_t* buff,
                   const size_t width,
                   const size_t height,
                   const uint16_t window_y1,
                   const uint16_t window_y2);

    bool RasterizePixel(Shape& shape,
                        uint8_t* buff,
                        const size_t width,
                        const size_t height,
                        const uint16_t window_y1,
                        const uint16_t window_y2);

    bool RasterizeLine(Shape& shape,
                       uint8_t* buff,
                       const size_t width,
                       const size_t height,
                       const uint16_t window_y1,
                       const uint16_t window_y2);

    bool RasterizeRectangle(Shape& shape,
                            uint8_t* buff,
                            const size_t width,
                            const size_t height,
                            const uint16_t window_y1,
                            const uint16_t window_y2);

    bool RasterizeCircle(Shape& shape,
                         uint8_t* buff,
                         const size_t width,
                         const size_t height,
                         const uint16_t window_y1,
                         const uint16_t window_y2);

    bool RasterizeString(Shape& shape,
                         uint8_t* buff,
                         const size_t width,
                         const size_t height,
                         const uint16_t window_y1,
                         const uint16_t window_y2);

    bool RasterizeBitmap(Shape& shape,
                         uint8_t* buff,
                         const size_t width,
                         const size_t height,
                         const uint16_t window_y1,
                         const uint16_t window_y2);

    inline void SetPixel(uint8_t* buff,
                         const size_t width,
                         const uint16_t x,
                         const uint16_t y,
                         const uint8_t colour_high,
                         const uint8_t colour_low);
};

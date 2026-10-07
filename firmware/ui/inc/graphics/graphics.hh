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
        FillRectangle,
        Rectangle,
        FillCircle,
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
        };

        ShapeType type;

        union
        {
            struct
            {
                const uint16_t x;
                const uint16_t y;
                const Colour colour;
            } pixel;

            struct
            {
                const uint16_t x1;
                const uint16_t y1;
                const uint16_t x2;
                const uint16_t y2;
                const Colour colour;
                uint8_t flags;
            } line;

            struct
            {
                const uint16_t x1;
                const uint16_t y1;
                const uint16_t x2;
                const uint16_t y2;
                const Colour colour;
                uint8_t flags;
            } fill_rectangle;

            struct
            {
                const uint16_t x1;
                const uint16_t y1;
                const uint16_t x2;
                const uint16_t y2;
                const uint16_t thickness;
                const Colour colour;
                uint8_t flags;
            } rectangle;

            struct
            {
                const uint16_t x;
                const uint16_t y;
                const uint16_t r;
                const Colour colour;
                uint8_t flags;
            } fill_circle;

            struct
            {
                const uint16_t x;
                const uint16_t y;
                const uint16_t r;
                const uint16_t thickness;
                const Colour colour;
                uint8_t flags;
            } circle;

            struct
            {
                const char* str;
                const uint16_t len;
                const uint16_t x;
                const uint16_t y;
                const Font& font;
                const Colour foreground;
                const Colour background;
                uint8_t flags;
            } string;

            struct
            {
                const uint16_t x;
                const uint16_t y;
                BitmapData* map;
            } bitmap;
        };
    };

    static bool Rasterize(Shape& shape,
                          uint8_t* buff,
                          const size_t width,
                          const size_t height,
                          const uint16_t window_y1,
                          const uint16_t window_y2);

    static bool RasterizePixel(Shape& shape,
                               uint8_t* buff,
                               const size_t width,
                               const size_t height,
                               const uint16_t window_y1,
                               const uint16_t window_y2);

    static bool RasterizeLine(Shape& shape,
                              uint8_t* buff,
                              const size_t width,
                              const size_t height,
                              const uint16_t window_y1,
                              const uint16_t window_y2);

    static bool RasterizeFillRectangle(Shape& shape,
                                       uint8_t* buff,
                                       const size_t width,
                                       const size_t height,
                                       const uint16_t window_y1,
                                       const uint16_t window_y2);

    static bool RasterizeRectangle(Shape& shape,
                                   uint8_t* buff,
                                   const size_t width,
                                   const size_t height,
                                   const uint16_t window_y1,
                                   const uint16_t window_y2);

    static bool RasterizeFillCircle(Shape& shape,
                                    uint8_t* buff,
                                    const size_t width,
                                    const size_t height,
                                    const uint16_t window_y1,
                                    const uint16_t window_y2);

    static bool RasterizeCircle(Shape& shape,
                                uint8_t* buff,
                                const size_t width,
                                const size_t height,
                                const uint16_t window_y1,
                                const uint16_t window_y2);

    static bool RasterizeString(Shape& shape,
                                uint8_t* buff,
                                const size_t width,
                                const size_t height,
                                const uint16_t window_y1,
                                const uint16_t window_y2);

    static bool RasterizeBitmap(Shape& shape,
                                uint8_t* buff,
                                const size_t width,
                                const size_t height,
                                const uint16_t window_y1,
                                const uint16_t window_y2);

private:
    static inline void SetPixel(uint8_t* buff,
                                const size_t width,
                                const uint16_t x,
                                const uint16_t y,
                                const uint8_t colour_high,
                                const uint8_t colour_low);
};

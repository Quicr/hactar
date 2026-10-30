#pragma once

#include "bitmap.hh"
#include "font.hh"
#include <cstdint>

class Graphics
{
public:
    // Pixel bounds with inclusive starts and exclusive ends.
    struct bound_t
    {
        uint16_t x1;
        uint16_t y1;
        uint16_t x2;
        uint16_t y2;
    };

    // Empty bounds and bounds that only touch at an edge do not intersect.
    static bool Intersecting(const bound_t& b1, const bound_t& b2);

    // Expand to using the minimum starts and maximum ends of both bounds.
    static void ExpandBound(bound_t& to, const bound_t& from);

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
        Bitmap,
        Mem
    };

    struct Shape
    {
        bool done = false;
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
                uint16_t thickness;
                Colour colour;
            } line;

            struct
            {
                uint16_t x1;
                uint16_t y1;
                uint16_t x2;
                uint16_t y2;
                Colour colour;
            } fill_rectangle;

            struct
            {
                uint16_t x1;
                uint16_t y1;
                uint16_t x2;
                uint16_t y2;
                uint16_t thickness;
                Colour colour;
            } rectangle;

            struct
            {
                uint16_t x;
                uint16_t y;
                uint16_t r;
                Colour colour;
            } fill_circle;

            struct
            {
                uint16_t x;
                uint16_t y;
                uint16_t r;
                uint16_t thickness;
                Colour colour;
            } circle;

            struct
            {
                const char* str;
                uint16_t len;
                uint16_t x;
                uint16_t y;
                const Font* font;
                Colour foreground;
                Colour background;
            } string;

            struct
            {
                uint16_t x;
                uint16_t y;
                const Bitmap* map;
                Colour foreground;
                Colour background;
            } bitmap;

            struct
            {
                uint16_t x1;
                uint16_t x2;
                uint16_t dest_y;
                uint16_t src_y1;
                uint16_t src_y2;
                uint16_t y_step;
            } mem;
        };
    };

    // Conservative pixel bounds, clamped to [0, UINT16_MAX]. Empty shapes return {}.
    // These are geometric bounds, not byte-aligned framebuffer access bounds.
    static bound_t GetBounds(Shape& shape);
    static bound_t GetPixelBounds(Shape& shape);
    static bound_t GetLineBounds(Shape& shape);
    static bound_t GetFillRectangleBounds(Shape& shape);
    static bound_t GetRectangleBounds(Shape& shape);
    static bound_t GetFillCircleBounds(Shape& shape);
    static bound_t GetCircleBounds(Shape& shape);
    static bound_t GetStringBounds(Shape& shape);
    static bound_t GetBitmapBounds(Shape& shape);
    static bound_t GetMemBounds(Shape& shape);

    static void Rasterize(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizePixel(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void RasterizeLine(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeFillRectangle(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeRectangle(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeFillCircle(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeCircle(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeString(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void
    RasterizeBitmap(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

    static void RasterizeMem(Shape& shape, uint8_t* buff, const size_t width, const size_t height);

private:
    static inline void SetPixel(uint8_t* buff,
                                const size_t width,
                                const uint16_t x,
                                const uint16_t y,
                                const uint8_t colour_high,
                                const uint8_t colour_low);

    static inline void FillRectangle(uint8_t* buff,
                                     const size_t width,
                                     const size_t height,
                                     const uint16_t x1,
                                     const uint16_t y1,
                                     const uint16_t x2,
                                     const uint16_t y2,
                                     const Colour colour);
};

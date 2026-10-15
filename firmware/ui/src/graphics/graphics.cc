#include "graphics.hh"
#include "logger.hh"
#include <algorithm>
#include <cmath>
#include <cstdint>

// TODO quantization needs to be included so we can get the actual width and height?
bool Graphics::Rasterize(Shape& shape,
                         uint8_t* buff,
                         const size_t width,
                         const size_t height,
                         const uint16_t window_y1,
                         const uint16_t window_y2)
{
    switch (shape.type)
    {
    case Graphics::ShapeType::Pixel:
    {
        return RasterizePixel(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::Line:
    {
        return RasterizeLine(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::FillRectangle:
    {
        return RasterizeFillRectangle(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::Rectangle:
    {
        return RasterizeRectangle(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::FillCircle:
    {
        return RasterizeFillCircle(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::Circle:
    {
        return RasterizeCircle(shape, buff, width, height, window_y1, window_y2);
    }
    case Graphics::ShapeType::String:
    {
        return RasterizeString(shape, buff, width, height, window_y1, window_y2);
    }
    default:
    {
        UI_LOG_ERROR("Graphics::Rasterize: Unknown shape type %d", (int)shape.type);
        return false;
    }
    }
}

bool Graphics::RasterizeFillRectangle(Shape& shape,
                                      uint8_t* buff,
                                      const size_t width,
                                      const size_t height,
                                      const uint16_t window_y1,
                                      const uint16_t window_y2)
{
    if (window_y1 > shape.fill_rectangle.y2 || window_y2 < shape.fill_rectangle.y1)
    {
        return false;
    }
    UI_LOG_INFO("fill_rectangle");

    const uint16_t y1 = std::max(shape.fill_rectangle.y1, window_y1);
    const uint16_t y2 = std::min(shape.fill_rectangle.y2, window_y2);
    const uint16_t x2 = std::min(shape.fill_rectangle.x2, static_cast<uint16_t>(width * 2));

    FillRectangle(buff, width, height, shape.fill_rectangle.x1, y1, x2, y2,
                  shape.fill_rectangle.colour);

    return shape.fill_rectangle.y2 >= y2;
}

bool Graphics::RasterizeRectangle(Shape& shape,
                                  uint8_t* buff,
                                  const size_t width,
                                  const size_t height,
                                  const uint16_t window_y1,
                                  const uint16_t window_y2)
{
    const auto& rectangle = shape.rectangle;
    if (buff == nullptr || rectangle.thickness == 0 || rectangle.x1 >= rectangle.x2
        || rectangle.y1 >= rectangle.y2)
    {
        return false;
    }

    const size_t x2 = std::min(static_cast<size_t>(rectangle.x2), width * 2);
    const size_t y1 = std::max(static_cast<size_t>(rectangle.y1), static_cast<size_t>(window_y1));
    const size_t y2 =
        std::min({static_cast<size_t>(rectangle.y2), static_cast<size_t>(window_y2), height});

    if (rectangle.x1 >= x2 || y1 >= y2)
    {
        return false;
    }

    const uint8_t colour_high = static_cast<uint8_t>(rectangle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(rectangle.colour) & 0x0F;

    for (size_t y = y1; y < y2; ++y)
    {
        for (size_t x = rectangle.x1; x < x2; ++x)
        {
            // Test against the original edges so clipping does not create new borders.
            if (x - rectangle.x1 < rectangle.thickness
                || rectangle.x2 - 1U - x < rectangle.thickness
                || y - rectangle.y1 < rectangle.thickness
                || rectangle.y2 - 1U - y < rectangle.thickness)
            {
                SetPixel(buff, width, static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                         colour_high, colour_low);
            }
        }
    }

    return true;
}

bool Graphics::RasterizePixel(Shape& shape,
                              uint8_t* buff,
                              const size_t width,
                              const size_t height,
                              const uint16_t window_y1,
                              const uint16_t window_y2)
{
    const auto& pixel = shape.pixel;
    if (buff == nullptr || pixel.x >= width * 2 || pixel.y >= height || pixel.y < window_y1
        || pixel.y >= window_y2)
    {
        return false;
    }

    const uint8_t colour_high = static_cast<uint8_t>(pixel.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(pixel.colour) & 0x0F;
    SetPixel(buff, width, pixel.x, pixel.y, colour_high, colour_low);
    return true;
}

bool Graphics::RasterizeLine(Shape& shape,
                             uint8_t* buff,
                             const size_t width,
                             const size_t height,
                             const uint16_t window_y1,
                             const uint16_t window_y2)
{
    const auto& line = shape.line;
    if (buff == nullptr || window_y1 >= window_y2 || line.thickness == 0)
    {
        return false;
    }

    int32_t x = line.x1;
    int32_t y = line.y1;
    const int32_t end_x = line.x2;
    const int32_t end_y = line.y2;
    const int32_t dx = end_x > x ? end_x - x : x - end_x;
    const int32_t dy = end_y > y ? end_y - y : y - end_y;
    const int32_t step_x = x < end_x ? 1 : -1;
    const int32_t step_y = y < end_y ? 1 : -1;
    int32_t error = dx - dy;

    const int32_t pixel_width = static_cast<int32_t>(std::min(width, size_t{32768}) * 2);
    const int32_t pixel_height = static_cast<int32_t>(std::min(height, size_t{65536}));
    const int32_t offset = -static_cast<int32_t>((line.thickness - 1) / 2);
    const uint8_t colour_high = static_cast<uint8_t>(line.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(line.colour) & 0x0F;
    bool drawn = false;

    for (;;)
    {
        // A shallow line gets a vertical span; a steep line gets a horizontal one.
        if (dx >= dy && x < pixel_width)
        {
            const int32_t first = std::max<int32_t>({0, window_y1, y + offset});
            const int32_t last =
                std::min<int32_t>({pixel_height, window_y2, y + offset + line.thickness});
            for (int32_t row = first; row < last; ++row)
            {
                SetPixel(buff, width, static_cast<uint16_t>(x), static_cast<uint16_t>(row),
                         colour_high, colour_low);
                drawn = true;
            }
        }
        else if (dy > dx && y >= window_y1 && y < window_y2 && y < pixel_height)
        {
            const int32_t first = std::max<int32_t>(0, x + offset);
            const int32_t last = std::min(pixel_width, x + offset + line.thickness);
            for (int32_t col = first; col < last; ++col)
            {
                SetPixel(buff, width, static_cast<uint16_t>(col), static_cast<uint16_t>(y),
                         colour_high, colour_low);
                drawn = true;
            }
        }

        if (x == end_x && y == end_y)
        {
            break;
        }

        const int32_t twice_error = 2 * error;
        if (twice_error > -dy)
        {
            error -= dy;
            x += step_x;
        }
        if (twice_error < dx)
        {
            error += dx;
            y += step_y;
        }
    }

    return drawn;
}

bool Graphics::RasterizeFillCircle(Shape& shape,
                                   uint8_t* buff,
                                   const size_t width,
                                   const size_t height,
                                   const uint16_t window_y1,
                                   const uint16_t window_y2)
{
    if (buff == nullptr || window_y1 >= window_y2)
    {
        return false;
    }

    const auto& circle = shape.fill_circle;

    const size_t fp_width = width * 2;

    const int16_t x1 = std::max(circle.x - circle.r, 0);
    const int16_t x2 = std::min<int16_t>(circle.x + circle.r, fp_width);
    const int16_t y1 = std::max(circle.y - circle.r, 0);
    const int16_t y2 = std::min<int16_t>({circle.y + circle.r, height, window_y2});

    const uint8_t colour_high = static_cast<uint8_t>(circle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(circle.colour) & 0x0F;

    const int16_t r_2 = circle.r * circle.r;

    for (int16_t y = y1 + 1; y < y2; ++y)
    {
        const int16_t v_dis =
            (y - y1 - static_cast<int16_t>(circle.r)) * (y - y1 - static_cast<int16_t>(circle.r));

        for (int16_t x = x1 + 1; x < x2; ++x)
        {
            const int16_t h_dis = (x - x1 - static_cast<int16_t>(circle.r))
                                * (x - x1 - static_cast<int16_t>(circle.r));

            if (h_dis + v_dis <= r_2)
            {
                SetPixel(buff, width, x, y, colour_high, colour_low);
            }
        }
    }

    return true;
}

bool Graphics::RasterizeCircle(Shape& shape,
                               uint8_t* buff,
                               const size_t width,
                               const size_t height,
                               const uint16_t window_y1,
                               const uint16_t window_y2)
{
    if (buff == nullptr || window_y1 >= window_y2 || shape.circle.thickness == 0)
    {
        return false;
    }

    const auto& circle = shape.circle;

    const size_t fp_width = width * 2;

    const int16_t x1 = std::max(circle.x - circle.r, 0);
    const int16_t x2 = std::min<int16_t>(circle.x + circle.r, fp_width);
    const int16_t y1 = std::max(circle.y - circle.r, 0);
    const int16_t y2 = std::min<int16_t>({circle.y + circle.r, height, window_y2});

    const uint8_t colour_high = static_cast<uint8_t>(circle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(circle.colour) & 0x0F;

    const int16_t thickness = std::min(circle.r, circle.thickness);
    const int16_t outer_r_2 = circle.r * circle.r;
    const int16_t inner_r_2 = (circle.r - thickness) * (circle.r - thickness);

    for (int16_t y = y1 + 1; y < y2; ++y)
    {
        const int16_t dy =
            (y - y1 - static_cast<int16_t>(circle.r)) * (y - y1 - static_cast<int16_t>(circle.r));

        for (int16_t x = x1 + 1; x < x2; ++x)
        {
            const int16_t dx = (x - x1 - static_cast<int16_t>(circle.r))
                             * (x - x1 - static_cast<int16_t>(circle.r));

            const int32_t distance = dx + dy;

            if (distance <= outer_r_2 && distance >= inner_r_2)
            {
                SetPixel(buff, width, x, y, colour_high, colour_low);
            }
        }
    }

    return true;
}

bool Graphics::RasterizeString(Shape& shape,
                               uint8_t* buff,
                               const size_t width,
                               const size_t height,
                               const uint16_t window_y1,
                               const uint16_t window_y2)
{
    const auto& text = shape.string;
    if (buff == nullptr || text.str == nullptr || text.font.data == nullptr || text.len == 0
        || text.font.width == 0 || text.font.height == 0)
    {
        return false;
    }

    // Intersect the string's pixel bounds with this render window and the buffer dimensions.
    const size_t y1 = std::max(static_cast<size_t>(text.y), static_cast<size_t>(window_y1));
    const size_t y2 = std::min(
        {static_cast<size_t>(text.y) + text.font.height, static_cast<size_t>(window_y2), height});
    const size_t x2 = std::min(static_cast<size_t>(text.x) + text.len * text.font.width, width * 2);

    if (y1 >= y2 || text.x >= x2)
    {
        return false;
    }

    const size_t bytes_per_ch = (text.font.width + 7) / 8;

    const uint8_t fg_high = static_cast<uint8_t>(text.foreground) << 4;
    const uint8_t fg_low = static_cast<uint8_t>(text.foreground) & 0x0F;
    const uint8_t bg_high = static_cast<uint8_t>(text.background) << 4;
    const uint8_t bg_low = static_cast<uint8_t>(text.background) & 0x0F;
    bool foreground = false;

    for (size_t y = y1; y < y2; ++y)
    {
        const size_t pixel_y_offset = y - text.y;
        for (size_t x = text.x; x < x2; ++x)
        {
            const size_t pixel_x_offset = x - text.x;
            const size_t font_col = pixel_x_offset % text.font.width;

            const size_t char_idx = pixel_x_offset / text.font.width;
            const uint8_t ch = static_cast<uint8_t>(text.str[char_idx]);

            if (ch >= 32 && ch <= 126)
            {
                // Find the glyph's row byte, then test its column's bit from left to right.
                // We find the character offset starting at ' ', and map it from zero.
                // then we multiply by the number of rows per character gets
                // plus the current row we are rasterizing, times of the offset for the number
                // of bytes per character plus the offset of which byte we are reading from for
                // a character
                const size_t byte_idx =
                    (static_cast<size_t>(ch - 32) * text.font.height + pixel_y_offset)
                        * bytes_per_ch
                    + font_col / 8;
                foreground = (text.font.data[byte_idx] & (0x80U >> (font_col % 8))) != 0;

                if (foreground)
                {
                    SetPixel(buff, width, static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                             fg_high, fg_low);
                }
                else
                {
                    SetPixel(buff, width, static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                             bg_high, bg_low);
                }
            }
        }
    }

    return true;
}

inline void Graphics::SetPixel(uint8_t* buff,
                               const size_t width,
                               const uint16_t x,
                               const uint16_t y,
                               const uint8_t colour_high,
                               const uint8_t colour_low)
{
    const uint16_t idx = x / 2;
    const size_t addr = idx + y * width;
    uint8_t& pixel = buff[addr];

    // Odd pixels is in the low bits, even pixels in the high bits
    if (x & 0x0001)
    {
        pixel = (pixel & 0xF0) | colour_low;
    }
    else
    {
        pixel = (pixel & 0x0F) | colour_high;
    }
}

inline void Graphics::FillRectangle(uint8_t* buff,
                                    const size_t width,
                                    const size_t height,
                                    const uint16_t x1,
                                    const uint16_t y1,
                                    const uint16_t x2,
                                    const uint16_t y2,
                                    const Colour colour)
{
    const uint8_t colour_high = static_cast<uint8_t>(colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(colour) & 0x0F;

    for (uint16_t y = y1; y < y2; ++y)
    {
        for (uint16_t x = x1; x < x2; ++x)
        {
            SetPixel(buff, width, x, y, colour_high, colour_low);
        }
    }
}

#include "graphics.hh"
#include "logger.hh"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
Graphics::bound_t MakeBounds(int64_t x1, int64_t y1, int64_t x2, int64_t y2)
{
    const auto clamp = [](int64_t value) {
        return static_cast<uint16_t>(std::clamp<int64_t>(value, 0, UINT16_MAX));
    };
    const Graphics::bound_t bounds = {clamp(x1), clamp(y1), clamp(x2), clamp(y2)};
    return bounds.x1 < bounds.x2 && bounds.y1 < bounds.y2 ? bounds : Graphics::bound_t{};
}
} // namespace

bool Graphics::Intersecting(const bound_t& b1, const bound_t& b2)
{
    return b1.x1 < b1.x2 && b1.y1 < b1.y2 && b2.x1 < b2.x2 && b2.y1 < b2.y2 && b1.x1 < b2.x2
        && b2.x1 < b1.x2 && b1.y1 < b2.y2 && b2.y1 < b1.y2;
}

void Graphics::ExpandBound(bound_t& to, const bound_t& from)
{
    if (from.x1 >= from.x2 || from.y1 >= from.y2)
    {
        return;
    }

    if (to.x1 >= to.x2 || to.y1 >= to.y2)
    {
        to = from;
        return;
    }

    to.x1 = std::min<uint16_t>(to.x1, from.x1);
    to.y1 = std::min<uint16_t>(to.y1, from.y1);
    to.x2 = std::max<uint16_t>(to.x2, from.x2);
    to.y2 = std::max<uint16_t>(to.y2, from.y2);
}

Graphics::bound_t Graphics::GetBounds(Shape& shape)
{
    switch (shape.type)
    {
    case ShapeType::Pixel:
        return GetPixelBounds(shape);
    case ShapeType::Line:
        return GetLineBounds(shape);
    case ShapeType::FillRectangle:
        return GetFillRectangleBounds(shape);
    case ShapeType::Rectangle:
        return GetRectangleBounds(shape);
    case ShapeType::FillCircle:
        return GetFillCircleBounds(shape);
    case ShapeType::Circle:
        return GetCircleBounds(shape);
    case ShapeType::String:
        return GetStringBounds(shape);
    case ShapeType::Bitmap:
        return GetBitmapBounds(shape);
    case ShapeType::Mem:
        return GetMemBounds(shape);
    default:
        return {};
    }
}

Graphics::bound_t Graphics::GetPixelBounds(Shape& shape)
{
    const auto& pixel = shape.pixel;
    return MakeBounds(pixel.x, pixel.y, int64_t{pixel.x} + 1, int64_t{pixel.y} + 1);
}

Graphics::bound_t Graphics::GetLineBounds(Shape& shape)
{
    const auto& line = shape.line;
    if (line.thickness == 0)
    {
        return {};
    }

    int64_t x1 = std::min<int64_t>(line.x1, line.x2);
    int64_t y1 = std::min<int64_t>(line.y1, line.y2);
    int64_t x2 = std::max<int64_t>(line.x1, line.x2) + 1;
    int64_t y2 = std::max<int64_t>(line.y1, line.y2) + 1;
    const int64_t offset = -int64_t{(line.thickness - 1) / 2};

    // Match the vertical/horizontal spans used by RasterizeLine.
    if (x2 - x1 >= y2 - y1)
    {
        y1 += offset;
        y2 += offset + line.thickness - 1;
    }
    else
    {
        x1 += offset;
        x2 += offset + line.thickness - 1;
    }
    return MakeBounds(x1, y1, x2, y2);
}

Graphics::bound_t Graphics::GetFillRectangleBounds(Shape& shape)
{
    const auto& rectangle = shape.fill_rectangle;
    return MakeBounds(rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y2);
}

Graphics::bound_t Graphics::GetRectangleBounds(Shape& shape)
{
    const auto& rectangle = shape.rectangle;
    if (rectangle.thickness == 0)
    {
        return {};
    }
    return MakeBounds(rectangle.x1, rectangle.y1, rectangle.x2, rectangle.y2);
}

Graphics::bound_t Graphics::GetFillCircleBounds(Shape& shape)
{
    const auto& circle = shape.fill_circle;
    if (circle.r == 0)
    {
        return {};
    }
    return MakeBounds(int64_t{circle.x} - circle.r, int64_t{circle.y} - circle.r,
                      int64_t{circle.x} + circle.r + 1, int64_t{circle.y} + circle.r + 1);
}

Graphics::bound_t Graphics::GetCircleBounds(Shape& shape)
{
    const auto& circle = shape.circle;
    if (circle.r == 0 || circle.thickness == 0)
    {
        return {};
    }
    return MakeBounds(int64_t{circle.x} - circle.r, int64_t{circle.y} - circle.r,
                      int64_t{circle.x} + circle.r + 1, int64_t{circle.y} + circle.r + 1);
}

Graphics::bound_t Graphics::GetStringBounds(Shape& shape)
{
    const auto& text = shape.string;
    if (text.str == nullptr || text.font == nullptr || text.font->data == nullptr || text.len == 0)
    {
        return {};
    }
    return MakeBounds(text.x, text.y, int64_t{text.x} + int64_t{text.len} * text.font->width,
                      int64_t{text.y} + text.font->height);
}

Graphics::bound_t Graphics::GetBitmapBounds(Shape& shape)
{
    const auto& bitmap = shape.bitmap;
    if (bitmap.map == nullptr || bitmap.map->data == nullptr)
    {
        return {};
    }
    return MakeBounds(bitmap.x, bitmap.y,
                      bitmap.x + std::min<size_t>(bitmap.map->width, UINT16_MAX),
                      bitmap.y + std::min<size_t>(bitmap.map->height, UINT16_MAX));
}

Graphics::bound_t Graphics::GetMemBounds(Shape& shape)
{
    const auto& mem = shape.mem;

    return MakeBounds(mem.x1, mem.dest_y, mem.x2, mem.src_y2);
}

// TODO quantization needs to be included so we can get the actual width and height?
void Graphics::Rasterize(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    switch (shape.type)
    {
    case Graphics::ShapeType::Pixel:
    {
        return RasterizePixel(shape, buff, width, height);
    }
    case Graphics::ShapeType::Line:
    {
        return RasterizeLine(shape, buff, width, height);
    }
    case Graphics::ShapeType::FillRectangle:
    {
        return RasterizeFillRectangle(shape, buff, width, height);
    }
    case Graphics::ShapeType::Rectangle:
    {
        return RasterizeRectangle(shape, buff, width, height);
    }
    case Graphics::ShapeType::FillCircle:
    {
        return RasterizeFillCircle(shape, buff, width, height);
    }
    case Graphics::ShapeType::Circle:
    {
        return RasterizeCircle(shape, buff, width, height);
    }
    case Graphics::ShapeType::String:
    {
        return RasterizeString(shape, buff, width, height);
    }
    case Graphics::ShapeType::Bitmap:
    {
        return RasterizeBitmap(shape, buff, width, height);
    }
    case Graphics::ShapeType::Mem:
    {
        return RasterizeMem(shape, buff, width, height);
    }
    default:
    {
        UI_LOG_ERROR("Graphics::Rasterize: Unknown shape type %d", (int)shape.type);
        return;
    }
    }
}

void Graphics::RasterizeFillRectangle(Shape& shape,
                                      uint8_t* buff,
                                      const size_t width,
                                      const size_t height)
{
    const uint16_t y1 = std::max<uint16_t>(shape.fill_rectangle.y1, 0);
    const uint16_t y2 = std::min<uint16_t>(shape.fill_rectangle.y2, height);
    const uint16_t x2 = std::min<uint16_t>(shape.fill_rectangle.x2, width * 2);

    FillRectangle(buff, width, height, shape.fill_rectangle.x1, y1, x2, y2,
                  shape.fill_rectangle.colour);

    shape.done = true;
}

void Graphics::RasterizeRectangle(Shape& shape,
                                  uint8_t* buff,
                                  const size_t width,
                                  const size_t height)
{
    const auto& rectangle = shape.rectangle;
    if (buff == nullptr || rectangle.thickness == 0 || rectangle.x1 >= rectangle.x2
        || rectangle.y1 >= rectangle.y2)
    {
        return;
    }

    const size_t x2 = std::min<size_t>(rectangle.x2, width * 2);
    const size_t y1 = std::max<size_t>(rectangle.y1, 0);
    const size_t y2 = std::min<size_t>(rectangle.y2, height);

    if (rectangle.x1 >= x2 || y1 >= y2)
    {
        return;
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
                SetPixel(buff, width, x, y, colour_high, colour_low);
            }
        }
    }

    shape.done = true;
}

void Graphics::RasterizePixel(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    const auto& pixel = shape.pixel;
    if (buff == nullptr || pixel.x >= width * 2 || pixel.y >= height)
    {
        return;
    }

    const uint8_t colour_high = static_cast<uint8_t>(pixel.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(pixel.colour) & 0x0F;
    SetPixel(buff, width, pixel.x, pixel.y, colour_high, colour_low);
    shape.done = true;
}

void Graphics::RasterizeLine(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    const auto& line = shape.line;
    if (buff == nullptr || height == 0 || line.thickness == 0)
    {
        return;
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

    const int32_t pixel_width = std::min<size_t>(width, 32768) * 2;
    const int32_t pixel_height = std::min<size_t>(height, 65536);
    const int32_t offset = -((line.thickness - 1) / 2);
    const uint8_t colour_high = static_cast<uint8_t>(line.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(line.colour) & 0x0F;

    for (;;)
    {
        // A shallow line gets a vertical span; a steep line gets a horizontal one.
        if (dx >= dy && x < pixel_width)
        {
            const int32_t first = std::max<int32_t>(0, y + offset);
            const int32_t last = std::min<int32_t>(pixel_height, y + offset + line.thickness);
            for (int32_t row = first; row < last; ++row)
            {
                SetPixel(buff, width, x, row, colour_high, colour_low);
            }
        }
        else if (dy > dx && y >= 0 && y < pixel_height)
        {
            const int32_t first = std::max<int32_t>(0, x + offset);
            const int32_t last = std::min<int32_t>(pixel_width, x + offset + line.thickness);
            for (int32_t col = first; col < last; ++col)
            {
                SetPixel(buff, width, col, y, colour_high, colour_low);
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

    shape.done = true;
}

void Graphics::RasterizeFillCircle(Shape& shape,
                                   uint8_t* buff,
                                   const size_t width,
                                   const size_t height)
{
    if (buff == nullptr || height == 0)
    {
        return;
    }

    const auto& circle = shape.fill_circle;

    const size_t fp_width = width * 2;

    const int16_t radii = std::min<int16_t>(circle.r, 180); // 181 overflows int16_t when doubled.
    const int16_t x1 = std::max<int16_t>(circle.x - radii, 0);
    const int16_t x2 = std::min<int16_t>(circle.x + radii, fp_width);
    const int16_t y1 = std::max<int16_t>(circle.y - radii, 0);
    const int16_t y2 = std::min<int16_t>(circle.y + radii, height);

    const uint8_t colour_high = static_cast<uint8_t>(circle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(circle.colour) & 0x0F;

    const int16_t r_2 = radii * radii;

    for (int16_t y = y1 + 1; y < y2; ++y)
    {
        const int16_t v_dis = (y - y1 - radii) * (y - y1 - radii);

        for (int16_t x = x1 + 1; x < x2; ++x)
        {
            const int16_t h_dis = (x - x1 - radii) * (x - x1 - radii);

            if (h_dis + v_dis <= r_2)
            {
                SetPixel(buff, width, x, y, colour_high, colour_low);
            }
        }
    }

    shape.done = true;
}

void Graphics::RasterizeCircle(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    if (buff == nullptr || height == 0 || shape.circle.thickness == 0)
    {
        return;
    }

    const auto& circle = shape.circle;

    const size_t fp_width = width * 2;

    const int16_t x1 = std::max<int16_t>(circle.x - circle.r, 0);
    const int16_t x2 = std::min<int16_t>(circle.x + circle.r, fp_width);
    const int16_t y1 = std::max<int16_t>(circle.y - circle.r, 0);
    const int16_t y2 = std::min<int16_t>(circle.y + circle.r, height);

    const uint8_t colour_high = static_cast<uint8_t>(circle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(circle.colour) & 0x0F;

    const int16_t thickness = std::min<int16_t>(circle.r, circle.thickness);
    const int16_t outer_r_2 = circle.r * circle.r;
    const int16_t inner_r_2 = (circle.r - thickness) * (circle.r - thickness);

    for (int16_t y = y1 + 1; y < y2; ++y)
    {
        const int16_t dy = (y - y1 - circle.r) * (y - y1 - circle.r);

        for (int16_t x = x1 + 1; x < x2; ++x)
        {
            const int16_t dx = (x - x1 - circle.r) * (x - x1 - circle.r);

            const int32_t distance = dx + dy;

            if (distance <= outer_r_2 && distance >= inner_r_2)
            {
                SetPixel(buff, width, x, y, colour_high, colour_low);
            }
        }
    }

    shape.done = true;
}

void Graphics::RasterizeString(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    const auto& text = shape.string;
    if (buff == nullptr || text.str == nullptr || text.font == nullptr || text.font->data == nullptr
        || text.len == 0 || text.font->width == 0 || text.font->height == 0)
    {
        return;
    }

    // Intersect the string's pixel bounds with the buffer dimensions.
    const size_t y1 = text.y;
    const size_t y2 = std::min<size_t>(text.y + text.font->height, height);
    const size_t x2 = std::min<size_t>(text.x + text.len * text.font->width, width * 2);

    if (y1 >= y2 || text.x >= x2)
    {
        return;
    }

    const size_t bytes_per_ch = (text.font->width + 7) / 8;

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
            const size_t font_col = pixel_x_offset % text.font->width;

            const size_t char_idx = pixel_x_offset / text.font->width;
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
                    ((ch - 32) * text.font->height + pixel_y_offset) * bytes_per_ch + font_col / 8;
                foreground = (text.font->data[byte_idx] & (0x80U >> (font_col % 8))) != 0;

                if (foreground)
                {
                    SetPixel(buff, width, x, y, fg_high, fg_low);
                }
                else
                {
                    SetPixel(buff, width, x, y, bg_high, bg_low);
                }
            }
        }
    }

    shape.done = true;
}

void Graphics::RasterizeBitmap(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    const auto& bitmap = shape.bitmap;
    if (buff == nullptr || bitmap.map == nullptr || bitmap.map->data == nullptr
        || bitmap.map->width == 0 || bitmap.map->height == 0)
    {
        return;
    }

    // Intersect the bitmap's pixel bounds with the buffer dimensions.
    const size_t y1 = bitmap.y;
    const size_t y2 = std::min<size_t>(bitmap.y + bitmap.map->height, height);
    const size_t x2 = std::min<size_t>(bitmap.x + bitmap.map->width, width * 2);

    if (y1 >= y2 || bitmap.x >= x2)
    {
        return;
    }

    const size_t bytes_per_row = (bitmap.map->width + 7) / 8;

    const uint8_t fg_high = static_cast<uint8_t>(bitmap.foreground) << 4;
    const uint8_t fg_low = static_cast<uint8_t>(bitmap.foreground) & 0x0F;
    const uint8_t bg_high = static_cast<uint8_t>(bitmap.background) << 4;
    const uint8_t bg_low = static_cast<uint8_t>(bitmap.background) & 0x0F;
    bool foreground = false;

    for (size_t y = y1; y < y2; ++y)
    {
        const size_t pixel_y_offset = y - bitmap.y;
        for (size_t x = bitmap.x; x < x2; ++x)
        {
            const size_t pixel_x_offset = x - bitmap.x;
            const size_t col_pixel = pixel_x_offset % bitmap.map->width;
            const size_t col_byte = col_pixel / 8;

            const size_t pixel_mask = col_pixel % 8;

            const size_t byte_idx = col_byte + pixel_y_offset * bytes_per_row;

            foreground = (bitmap.map->data[byte_idx] & (0x80U >> pixel_mask)) != 0;

            if (foreground)
            {
                SetPixel(buff, width, x, y, fg_high, fg_low);
            }
            else
            {
                SetPixel(buff, width, x, y, bg_high, bg_low);
            }
        }
    }

    shape.done = true;
}

void Graphics::RasterizeMem(Shape& shape, uint8_t* buff, const size_t width, const size_t height)
{
    const auto& mem = shape.mem;

    if (buff == nullptr || width == 0 || mem.src_y1 > mem.src_y2 || mem.src_y2 > height)
    {
        UI_LOG_ERROR("Graphics::RasterizeMem: invalid scroll range");
        return;
    }

    uint16_t dest_y = std::min<uint16_t>(mem.dest_y, height);

    // Scroll the colour data
    for (uint16_t src_y = mem.src_y1; src_y < mem.src_y2; src_y += mem.y_step)
    {
        if (dest_y >= height)
        {
            break;
        }

        std::memcpy(buff + dest_y * width, buff + src_y * width, mem.y_step * width);
        dest_y += mem.y_step;
    }

    std::memset(buff + (mem.src_y2 - mem.y_step) * width, 0, width * mem.y_step);

    shape.done = true;
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

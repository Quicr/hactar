#include "graphics.hh"
#include <algorithm>
#include <cmath>

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
    const int32_t radius = circle.r;
    const int32_t center_y = circle.y;
    const int32_t y1 = std::max<int32_t>({0, int32_t(window_y1), center_y - radius});
    const int32_t bottom = static_cast<int32_t>(std::min(height, size_t{65536}));
    const int32_t y2 = std::min({center_y + radius + 1, int32_t(window_y2), bottom});
    const int32_t pixel_width = static_cast<int32_t>(std::min(width, size_t{32768}) * 2);
    if (y1 >= y2 || pixel_width == 0)
    {
        return false;
    }

    const int64_t radius_squared = static_cast<int64_t>(radius) * radius;
    const uint8_t colour_high = static_cast<uint8_t>(circle.colour) << 4;
    const uint8_t colour_low = static_cast<uint8_t>(circle.colour) & 0x0F;
    bool drawn = false;

    for (int32_t y = y1; y < y2; ++y)
    {
        const int64_t dy = static_cast<int64_t>(y) - circle.y;
        const int64_t distance_squared = radius_squared - dy * dy;
        int32_t extent = static_cast<int32_t>(std::sqrt(static_cast<double>(distance_squared)));
        // Correct any rounding at exact squares before drawing the horizontal span.
        while (static_cast<int64_t>(extent + 1) * (extent + 1) <= distance_squared)
        {
            ++extent;
        }
        while (static_cast<int64_t>(extent) * extent > distance_squared)
        {
            --extent;
        }

        const int32_t x1 = std::max(int32_t{0}, static_cast<int32_t>(circle.x) - extent);
        const int32_t x2 = std::min(pixel_width, static_cast<int32_t>(circle.x) + extent + 1);
        for (int32_t x = x1; x < x2; ++x)
        {
            SetPixel(buff, width, static_cast<uint16_t>(x), static_cast<uint16_t>(y), colour_high,
                     colour_low);
            drawn = true;
        }
    }

    return drawn;
}

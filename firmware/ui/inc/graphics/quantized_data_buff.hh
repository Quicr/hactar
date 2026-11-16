#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class QuantizedBuffer
{
public:
    constexpr QuantizedBuffer(uint8_t* buffer,
                              uint16_t width,
                              uint16_t height,
                              uint16_t quantization) noexcept :
        buffer(buffer),
        width(width),
        height(height),
        quantization(quantization)
    {
    }

    uint8_t* const buffer;
    const uint16_t width;        // Bytes per row
    const uint16_t height;       // Pixel rows
    const uint16_t quantization; // Bits per pixel
};

template <uint16_t WIDTH, uint16_t HEIGHT, uint16_t QUANTIZATION>
class StaticQuantizedBuffer : std::array<uint8_t, ((WIDTH * QUANTIZATION + 7) / 8) * HEIGHT>,
                              public QuantizedBuffer
{
public:
    static_assert(WIDTH > 0 && HEIGHT > 0);
    static_assert(QUANTIZATION == 1 || QUANTIZATION == 2 || QUANTIZATION == 4 || QUANTIZATION == 8);

    static constexpr std::size_t ROW_BYTES = (std::size_t{WIDTH} * QUANTIZATION + 7) / 8;
    static_assert(ROW_BYTES <= UINT16_MAX);

    constexpr StaticQuantizedBuffer() noexcept :
        std::array<uint8_t, ((WIDTH * QUANTIZATION + 7) / 8) * HEIGHT>(),
        QuantizedBuffer(this->data(), static_cast<uint16_t>(ROW_BYTES), HEIGHT, QUANTIZATION)
    {
    }

    // Prevent copies from retaining a pointer into another object's storage.
    StaticQuantizedBuffer(const StaticQuantizedBuffer&) = delete;
    StaticQuantizedBuffer& operator=(const StaticQuantizedBuffer&) = delete;
    StaticQuantizedBuffer(StaticQuantizedBuffer&&) = delete;
    StaticQuantizedBuffer& operator=(StaticQuantizedBuffer&&) = delete;
};

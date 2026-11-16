#pragma once

#include "quantized_data_buff.hh"
#include "scanline_buff.hh"

class ScreenBuffers
{
public:
    constexpr ScreenBuffers(ScanlineBuffer* scanline, QuantizedBuffer* data) noexcept :
        scanline(scanline),
        data(data)
    {
    }

    ScanlineBuffer* const scanline;
    QuantizedBuffer* const data;
};

namespace screen_buffers_detail
{
template <uint16_t WIDTH,
          uint16_t HEIGHT,
          uint16_t QUANTIZATION,
          uint16_t NUM_SCAN_LINES,
          uint16_t NUM_SCANLINE_BUFFS>
struct Storage
{
    StaticScanLineBuffer<NUM_SCAN_LINES, WIDTH, NUM_SCANLINE_BUFFS> scanline_storage;
    StaticQuantizedBuffer<WIDTH, HEIGHT, QUANTIZATION> data_storage;
};
} // namespace screen_buffers_detail

template <uint16_t WIDTH,
          uint16_t HEIGHT,
          uint16_t QUANTIZATION,
          uint16_t NUM_SCAN_LINES,
          uint16_t NUM_SCANLINE_BUFFS>
class StaticScreenBuffers
    : private screen_buffers_detail::
          Storage<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>,
      public ScreenBuffers
{
    using Storage = screen_buffers_detail::
        Storage<WIDTH, HEIGHT, QUANTIZATION, NUM_SCAN_LINES, NUM_SCANLINE_BUFFS>;

public:
    constexpr StaticScreenBuffers() noexcept :
        Storage(),
        ScreenBuffers(&this->scanline_storage, &this->data_storage)
    {
    }

    StaticScreenBuffers(const StaticScreenBuffers&) = delete;
    StaticScreenBuffers& operator=(const StaticScreenBuffers&) = delete;
    StaticScreenBuffers(StaticScreenBuffers&&) = delete;
    StaticScreenBuffers& operator=(StaticScreenBuffers&&) = delete;
};

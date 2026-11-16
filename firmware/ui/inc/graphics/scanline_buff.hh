#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class ScanlineBuffer
{
public:
    constexpr ScanlineBuffer(uint16_t* buffer,
                             uint16_t num_lines,
                             uint16_t num_cols,
                             uint16_t num_buffs) noexcept :
        buffer(buffer),
        num_lines(num_lines),
        num_cols(num_cols),
        num_buffs(num_buffs)
    {
    }

    uint16_t* Current() const noexcept
    {
        return buffer + idx * num_lines * num_cols;
    }

    void Advance() noexcept
    {
        if (++idx >= num_buffs)
        {
            idx = 0;
        }
    }

    void Reset() noexcept
    {
        idx = 0;
    }

    uint16_t* const buffer;
    const uint16_t num_lines;
    const uint16_t num_cols;
    const uint16_t num_buffs;

    ~ScanlineBuffer() = default;

private:
    std::size_t idx = 0;
};

template <uint16_t NUM_LINES, uint16_t NUM_COLS, uint16_t NUM_BUFFS>
class StaticScanLineBuffer : std::array<uint16_t, NUM_LINES * NUM_COLS * NUM_BUFFS>,
                             public ScanlineBuffer
{
public:
    static_assert(NUM_LINES > 0 && NUM_COLS > 0 && NUM_BUFFS > 0);

    constexpr StaticScanLineBuffer() noexcept :
        std::array<uint16_t, NUM_LINES * NUM_COLS * NUM_BUFFS>(),
        ScanlineBuffer(this->data(), NUM_LINES, NUM_COLS, NUM_BUFFS)
    {
    }

    StaticScanLineBuffer(const StaticScanLineBuffer&) = delete;
    StaticScanLineBuffer& operator=(const StaticScanLineBuffer&) = delete;
    StaticScanLineBuffer(StaticScanLineBuffer&&) = delete;
    StaticScanLineBuffer& operator=(StaticScanLineBuffer&&) = delete;
};

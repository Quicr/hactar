#pragma once

#include <cstdint>
#include <span>

// Logical display operations shared by the board and host implementations.
class DisplayDriver
{
public:
    enum class Orientation : uint8_t
    {
        Portrait,
        Landscape,
        Portrait_Inverted,
        Landscape_Inverted,
    };

    virtual ~DisplayDriver() = default;

    virtual void Init() = 0;
    virtual void Reset() = 0;
    virtual void Sleep() = 0;
    virtual void Wake() = 0;
    virtual void EnableBacklight() = 0;
    virtual void DisableBacklight() = 0;
    virtual void Rotate(Orientation orientation) = 0;

    virtual uint16_t GetWidth() const = 0;
    virtual uint16_t GetHeight() const = 0;

    // Inclusive bounds. Successive writes advance through the region in row order.
    virtual void SetRenderRegion(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) = 0;

    // RGB565 bytes, most significant byte first (the firmware's scanline format).
    // False means no transfer was started. An asynchronous implementation may retain
    // the data until transfer completion; it must not invoke Screen callbacks inline.
    // Screen::Render prepares the next buffer and Screen::Transfer submits it once
    // the current transfer has completed. Hosts can pump these calls sequentially.
    virtual bool WritePixels(std::span<const uint8_t> pixels) = 0;
    virtual void EndWrite() = 0;
};

#pragma once

#include <cstdint>
#include <span>

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

    virtual void Init() = 0;
    virtual uint16_t GetWidth() const = 0;
    virtual uint16_t GetHeight() const = 0;
    virtual void Rotate(Orientation orientation) = 0;
    virtual void EnableBacklight() = 0;
    virtual void DisableBacklight() = 0;

    // Start a nonempty, half-open rectangle: [x1, x2) x [y1, y2).
    virtual void BeginRegion(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) = 0;

    // RGB565, high byte first. True means accepted, not necessarily complete.
    // Screen keeps the chunk intact until the transfer completion callback.
    virtual bool WritePixels(std::span<const uint8_t> pixels) = 0;

    // May also be called when idle; must leave the driver ready for a new region.
    virtual void EndRegion() = 0;

protected:
    // Screen borrows the driver; it does not delete it through this interface.
    ~DisplayDriver() = default;
};

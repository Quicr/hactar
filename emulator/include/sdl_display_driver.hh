#pragma once

#include "display_driver.hh"
#include <SDL.h>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace emulator
{
class SdlDisplayDriver final : public DisplayDriver
{
public:
    SdlDisplayDriver(uint16_t width, uint16_t height, int scale, bool hidden);
    ~SdlDisplayDriver() override;
    SdlDisplayDriver(const SdlDisplayDriver&) = delete;
    SdlDisplayDriver& operator=(const SdlDisplayDriver&) = delete;

    void Init() override;
    void Reset() override;
    void Sleep() override;
    void Wake() override;
    void EnableBacklight() override;
    void DisableBacklight() override;
    void Rotate(Orientation orientation) override;
    uint16_t GetWidth() const override;
    uint16_t GetHeight() const override;
    void SetRenderRegion(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) override;
    bool WritePixels(std::span<const uint8_t> pixels) override;
    void EndWrite() override;

    void Present();
    void SaveScreenshot(const std::string& path) const;

    // Native-endian RGB565 pixels in logical display coordinates, including while
    // the backlight is off. Useful for inspecting the real firmware's output.
    std::span<const uint16_t> Pixels() const noexcept;

private:
    bool Visible() const noexcept;
    void CreateTexture();

    const uint16_t width;
    const uint16_t height;
    const int scale;
    uint16_t view_width;
    uint16_t view_height;
    uint16_t region_x = 0;
    uint16_t region_y = 0;
    uint16_t region_width = 0;
    size_t region_size = 0;
    size_t region_offset = 0;
    bool backlight = false;
    bool sleeping = false;
    std::vector<uint16_t> pixels;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};
} // namespace emulator

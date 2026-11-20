#include "sdl_display_driver.hh"
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace emulator
{
namespace
{
std::runtime_error SdlError(const char* operation)
{
    return std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}
} // namespace

SdlDisplayDriver::SdlDisplayDriver(uint16_t width, uint16_t height, int scale, bool hidden) :
    width(width),
    height(height),
    scale(scale),
    view_width(width),
    view_height(height),
    pixels(size_t(width) * height)
{
    if (width == 0 || height == 0 || scale < 1 || scale > 8)
    {
        throw std::invalid_argument("Invalid display dimensions or scale (expected 1..8)");
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    window = SDL_CreateWindow("Hactar UI emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              width * scale, height * scale,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
                                  | (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    if (window == nullptr)
    {
        throw SdlError("Create window");
    }

    // SDL's dummy video driver and some remote desktops only provide software rendering.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (renderer == nullptr)
    {
        const auto error = SdlError("Create renderer");
        SDL_DestroyWindow(window);
        throw error;
    }

    try
    {
        CreateTexture();
    }
    catch (...)
    {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        throw;
    }
    Reset();
}

SdlDisplayDriver::~SdlDisplayDriver()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void SdlDisplayDriver::CreateTexture()
{
    SDL_Texture* next = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
                                          SDL_TEXTUREACCESS_STREAMING, view_width, view_height);
    if (next == nullptr)
    {
        throw SdlError("Create RGB565 texture");
    }
    SDL_DestroyTexture(texture);
    texture = next;
    if (SDL_RenderSetLogicalSize(renderer, view_width, view_height) != 0)
    {
        throw SdlError("Set logical display size");
    }
}

void SdlDisplayDriver::Init()
{
    Reset();
}

void SdlDisplayDriver::Reset()
{
    std::fill(pixels.begin(), pixels.end(), 0);
    sleeping = false;
    SetRenderRegion(0, 0, view_width - 1, view_height - 1);
}

void SdlDisplayDriver::Sleep()
{
    sleeping = true;
}

void SdlDisplayDriver::Wake()
{
    sleeping = false;
}

void SdlDisplayDriver::EnableBacklight()
{
    backlight = true;
}

void SdlDisplayDriver::DisableBacklight()
{
    backlight = false;
}

void SdlDisplayDriver::Rotate(Orientation orientation)
{
    const bool portrait =
        orientation == Orientation::Portrait || orientation == Orientation::Portrait_Inverted;
    view_width = portrait ? height : width;
    view_height = portrait ? width : height;
    CreateTexture();
    SDL_SetWindowSize(window, view_width * scale, view_height * scale);
    Reset();
}

uint16_t SdlDisplayDriver::GetWidth() const
{
    return view_width;
}

uint16_t SdlDisplayDriver::GetHeight() const
{
    return view_height;
}

void SdlDisplayDriver::SetRenderRegion(uint16_t x_start,
                                       uint16_t y_start,
                                       uint16_t x_end,
                                       uint16_t y_end)
{
    region_offset = 0;
    region_size = 0;
    if (x_start > x_end || y_start > y_end || x_end >= view_width || y_end >= view_height)
    {
        return;
    }
    region_x = x_start;
    region_y = y_start;
    region_width = x_end - x_start + 1;
    region_size = size_t(region_width) * (y_end - y_start + 1);
}

bool SdlDisplayDriver::WritePixels(std::span<const uint8_t> bytes)
{
    const size_t count = bytes.size() / sizeof(uint16_t);
    if (bytes.empty() || bytes.size() % sizeof(uint16_t) != 0
        || count > region_size - region_offset)
    {
        return false;
    }

    for (size_t i = 0; i < count; ++i)
    {
        const size_t offset = region_offset + i;
        const size_t x = region_x + offset % region_width;
        const size_t y = region_y + offset / region_width;
        pixels[y * view_width + x] = (uint16_t(bytes[2 * i]) << 8) | bytes[2 * i + 1];
    }
    region_offset += count;
    return true;
}

void SdlDisplayDriver::EndWrite()
{
    // Pixel writes complete synchronously; there is no chip select on the host.
    region_size = 0;
    region_offset = 0;
}

bool SdlDisplayDriver::Visible() const noexcept
{
    return backlight && !sleeping;
}

void SdlDisplayDriver::Present()
{
    if (SDL_UpdateTexture(texture, nullptr, pixels.data(), view_width * sizeof(uint16_t)) != 0)
    {
        throw SdlError("Upload display pixels");
    }
    const uint8_t brightness = Visible() ? 255 : 0;
    if (SDL_SetTextureColorMod(texture, brightness, brightness, brightness) != 0
        || SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 || SDL_RenderClear(renderer) != 0
        || SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0)
    {
        throw SdlError("Render display");
    }
    SDL_RenderPresent(renderer);
}

void SdlDisplayDriver::SaveScreenshot(const std::string& path) const
{
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface(
        SDL_CreateRGBSurfaceWithFormat(0, view_width, view_height, 24, SDL_PIXELFORMAT_RGB24),
        SDL_FreeSurface);
    if (surface == nullptr)
    {
        throw SdlError("Create screenshot surface");
    }

    if (Visible())
    {
        if (SDL_ConvertPixels(view_width, view_height, SDL_PIXELFORMAT_RGB565, pixels.data(),
                              view_width * sizeof(uint16_t), surface->format->format,
                              surface->pixels, surface->pitch)
            != 0)
        {
            throw SdlError("Convert screenshot pixels");
        }
    }
    else
    {
        SDL_FillRect(surface.get(), nullptr, 0);
    }
    if (SDL_SaveBMP(surface.get(), path.c_str()) != 0)
    {
        throw SdlError("Save screenshot");
    }
}

std::span<const uint16_t> SdlDisplayDriver::Pixels() const noexcept
{
    return pixels;
}
} // namespace emulator

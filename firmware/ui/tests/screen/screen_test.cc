#include "screen.hh"
#include <array>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace
{
constexpr uint16_t Width = 128;
constexpr uint16_t Height = 57;
constexpr uint16_t ChunkRows = 4;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class RecordingDriver final : public ScreenDriver
{
public:
    void Init() override
    {
        initialized = true;
    }
    uint16_t GetWidth() const override
    {
        return Width;
    }
    uint16_t GetHeight() const override
    {
        return Height;
    }
    void Rotate(Orientation) override
    {
    }
    void EnableBacklight() override
    {
        backlight = true;
    }
    void DisableBacklight() override
    {
        backlight = false;
    }

    void BeginRegion(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) override
    {
        Check(!active, "new region began before the previous one ended");
        Check(x1 == 0 && y1 == 0 && x2 == Width && y2 == Height, "bad half-open bounds");
        active = true;
        ++regions;
    }

    bool WritePixels(std::span<const uint8_t> bytes) override
    {
        Check(active && pending.empty(), "overlapping transfers");
        if (std::exchange(reject, false))
        {
            return false;
        }
        pending = bytes;
        return true;
    }

    void EndRegion() override
    {
        Check(active && pending.empty(), "ended an inactive or incomplete region");
        active = false;
        ++ended;
    }

    void CompleteChunk()
    {
        Check(!pending.empty(), "no transfer to complete");
        chunks.push_back(pending.size());
        pixels.insert(pixels.end(), pending.begin(), pending.end());
        pending = {};
    }

    std::span<const uint8_t> pending;
    std::vector<size_t> chunks;
    std::vector<uint8_t> pixels;
    unsigned regions = 0;
    unsigned ended = 0;
    bool reject = false;
    bool active = false;
    bool initialized = false;
    bool backlight = false;
};
} // namespace

int main()
{
    Battery battery;
    std::array<uint8_t, TileMap::GetSize(Width, Height, 16)> tiles{};
    TileMap tile_map(tiles.data(), tiles.size(), Width, Height, 16);
    RecordingDriver driver;
    StaticScreen<Width, Height, 4, ChunkRows, 2, JetBrainsMono15x20, JetBrainsMono10x16> screen(
        battery, tile_map, driver);
    screen.Init();
    screen.EnableBacklight();
    Check(driver.initialized && driver.backlight, "display init/backlight not forwarded");
    screen.DisableBacklight();
    Check(!driver.backlight, "backlight disable not forwarded");

    screen.FillScreen(Graphics::Colour::Red);
    Graphics::Shape bottom{.type = Graphics::ShapeType::FillRectangle,
                           .fill_rectangle = {.x1 = 0,
                                              .y1 = ChunkRows,
                                              .x2 = Width,
                                              .y2 = Height,
                                              .colour = Graphics::Colour::Green}};
    Check(screen.SubmitShape(bottom), "could not submit bottom rectangle");
    driver.reject = true;
    screen.Draw(0);
    Check(!screen.IsRendering() && driver.ended == 1, "rejected transfer did not close region");

    screen.Draw(0);
    for (unsigned i = 0; i < (Height + ChunkRows - 1) / ChunkRows; ++i)
    {
        Check(screen.IsRendering(), "render ended before final chunk");
        screen.Render(); // Simulate the DMA half-complete callback.
        driver.CompleteChunk();
        screen.Transfer(); // Simulate the DMA complete callback.
    }
    Check(!screen.IsRendering() && driver.regions == 2 && driver.ended == 2,
          "refresh did not retry and complete cleanly");
    Check(driver.chunks.size() == (Height + ChunkRows - 1) / ChunkRows
              && driver.chunks.front() == 2 * Width * ChunkRows
              && driver.chunks.back() == 2 * Width,
          "wrong scanline chunk sizes");
    Check(driver.pixels.size() == 2 * Width * Height, "wrong pixel byte count");
    for (size_t i = 0; i < Width * Height; ++i)
    {
        const bool red = i / Width < ChunkRows;
        Check(driver.pixels[2 * i] == (red ? 0xf8 : 0x07)
                  && driver.pixels[2 * i + 1] == (red ? 0x00 : 0xe0),
              "incorrect RGB565 byte order or scanline order");
    }
}

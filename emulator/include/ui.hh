#pragma once

#include "battery.hh"
#include "renderer.hh"
#include "screen.hh"
#include "tile_map.hh"
#include <array>
#include <cstdint>
#include <string_view>

namespace emulator
{
// Host composition of the firmware's screen and renderer. It owns no HAL objects.
class Ui
{
public:
    static constexpr uint16_t Width = 320;
    static constexpr uint16_t Height = 240;

    explicit Ui(DisplayDriver& display);
    Ui(const Ui&) = delete;
    Ui& operator=(const Ui&) = delete;

    void Tick(uint32_t ticks_ms);
    void ChangeView(Renderer::View view);
    void TextInput(std::string_view text);
    void Backspace();
    void CommitInput();
    Screen& GetScreen() noexcept;

private:
    static constexpr uint16_t Tile_Size = 16;
    Battery battery;
    std::array<uint8_t, TileMap::GetSize(Width, Height, Tile_Size)> tiles{};
    TileMap tile_map;
    StaticScreen<Width, Height, 4, 20, 2, JetBrainsMono15x20, JetBrainsMono10x16> screen;
    Renderer renderer;
    Renderer::View view = Renderer::View::Startup;
};
} // namespace emulator

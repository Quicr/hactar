#include "ui.hh"
#include <stdexcept>

namespace emulator
{
Ui::Ui(DisplayDriver& display) :
    tile_map(tiles.data(), tiles.size(), Width, Height, Tile_Size),
    screen(battery, tile_map, display),
    renderer(screen)
{
    if (display.GetWidth() != Width || display.GetHeight() != Height)
    {
        throw std::invalid_argument("The UI currently requires a 320x240 landscape display");
    }
    battery.ChargeLevel(100);
    screen.Init();
    screen.EnableBacklight();
}

void Ui::Tick(uint32_t ticks_ms)
{
    renderer.Render(ticks_ms);

    // The board prepares a scanline buffer at the DMA half-complete interrupt,
    // then submits it at transfer-complete. The host consumes pixels immediately
    // and advances the exact same pipeline here, without recursive callbacks.
    size_t transfers = 0;
    while (screen.IsRendering())
    {
        if (++transfers > size_t(Width) * Height)
        {
            throw std::runtime_error("Display transfer pipeline failed to drain");
        }
        screen.Render();
        screen.Transfer();
    }
}

void Ui::ChangeView(Renderer::View next)
{
    screen.ClearUserText();
    view = next;
    renderer.ChangeView(next);
}

void Ui::TextInput(std::string_view text)
{
    if (view != Renderer::View::Chat)
    {
        return;
    }
    // The firmware's bitmap fonts support printable ASCII, not UTF-8 glyphs.
    for (const unsigned char ch : text)
    {
        if (ch >= 32 && ch <= 126)
        {
            screen.AppendUserText(static_cast<char>(ch));
        }
    }
}

void Ui::Backspace()
{
    if (view == Renderer::View::Chat)
    {
        screen.BackspaceUserText();
    }
}

void Ui::CommitInput()
{
    if (view == Renderer::View::Chat && screen.UserTextLength() > 0)
    {
        // Local display exercise for this milestone; UART/network comes later.
        screen.CommitText(screen.UserText(), screen.UserTextLength());
        screen.ClearUserText();
    }
}

Screen& Ui::GetScreen() noexcept
{
    return screen;
}
} // namespace emulator

#include "sdl_display_driver.hh"
#include "ui.hh"
#include <SDL.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

uint16_t Pixel(const emulator::SdlDisplayDriver& display, uint16_t x, uint16_t y)
{
    return display.Pixels()[size_t(y) * display.GetWidth() + x];
}

std::vector<uint16_t> Rows(const emulator::SdlDisplayDriver& display, uint16_t y, uint16_t count)
{
    const auto pixels = display.Pixels();
    return {pixels.begin() + size_t(y) * display.GetWidth(),
            pixels.begin() + size_t(y + count) * display.GetWidth()};
}

void TestPixelStream(emulator::SdlDisplayDriver& display)
{
    display.Reset();
    display.SetRenderRegion(7, 9, 8, 10);
    constexpr std::array<uint8_t, 4> red_green = {0xF8, 0x00, 0x07, 0xE0};
    constexpr std::array<uint8_t, 4> blue_white = {0x00, 0x1F, 0xFF, 0xFF};
    constexpr std::array<uint8_t, 1> odd = {0xFF};
    Check(!display.WritePixels(odd), "Odd-sized RGB565 writes must be rejected");
    Check(display.WritePixels(red_green), "First partial write failed");
    Check(display.WritePixels(blue_white), "Second partial write failed");
    Check(Pixel(display, 7, 9) == 0xF800 && Pixel(display, 8, 9) == 0x07E0,
          "RGB565 byte order or region positioning is wrong");
    Check(Pixel(display, 7, 10) == 0x001F && Pixel(display, 8, 10) == 0xFFFF,
          "Successive writes must advance to the next row");
    Check(Pixel(display, 6, 9) == 0 && Pixel(display, 9, 10) == 0,
          "A partial transfer wrote outside its region");
    Check(!display.WritePixels(red_green), "Writes past the end of a region must fail");
    display.EndWrite();
    Check(!display.WritePixels(red_green), "Writes without an active region must fail");
    display.SetRenderRegion(319, 239, 320, 240);
    Check(!display.WritePixels(red_green), "Out-of-bounds regions must not accept pixels");
}

void TestFirmwareViews(emulator::SdlDisplayDriver& display)
{
    emulator::Ui ui(display);
    ui.Tick(0);
    Check(!ui.GetScreen().IsRendering(), "Startup transfers did not complete");
    const auto startup = Rows(display, 0, emulator::Ui::Height);
    Check(std::count(startup.begin(), startup.end(), 0xFFFF) > 1000,
          "The firmware splash bitmap was not rendered");
    Check(Pixel(display, 0, 0) == 0 && Pixel(display, 319, 239) == 0,
          "Splash margins should be black");

    ui.ChangeView(Renderer::View::Chat);
    ui.Tick(0);
    const auto chat = Rows(display, 0, emulator::Ui::Height);
    Check(chat != startup, "Changing views did not update the display");
    Check(Pixel(display, 0, 19) == 0xFFFF && Pixel(display, 319, 19) == 0xFFFF,
          "Firmware title separator is missing");
    Check(Pixel(display, 0, 222) == 0xFFFF && Pixel(display, 319, 222) == 0xFFFF,
          "Firmware input separator is missing");
    Check(std::count(chat.begin(), chat.begin() + 19 * 320, 0xFFFF) > 100,
          "Firmware title and status icons are missing");

    ui.TextInput("Hello!");
    ui.Backspace();
    Check(std::string(ui.GetScreen().UserText(), ui.GetScreen().UserTextLength()) == "Hello",
          "Host input did not reach the firmware's text editor");
    ui.CommitInput();
    ui.Tick(0);
    Check(ui.GetScreen().UserTextLength() == 0, "Committing must clear the input line");
    const auto first_line = Rows(display, 206, 16);
    Check(std::count(first_line.begin(), first_line.end(), 0xFFFF) > 0,
          "Committed input was not rasterized into chat");
    ui.TextInput("Second line");
    ui.CommitInput();
    ui.Tick(0);
    Check(Rows(display, 190, 16) == first_line, "Firmware chat scrolling lost the previous line");

    for (int i = 0; i < 40; ++i)
    {
        ui.TextInput(std::string(60, static_cast<char>('A' + i % 26)));
        Check(ui.GetScreen().UserTextLength() == 32, "Input must stop at the display width");
        ui.CommitInput();
        ui.Tick(0);
    }
    Check(Rows(display, 0, 20) == std::vector<uint16_t>(chat.begin(), chat.begin() + 20 * 320),
          "Scrolling overwrote the title or icons");
    Check(Pixel(display, 319, 222) == 0xFFFF, "Scrolling overwrote the input separator");

    // Exercise dirty-tile updates at odd coordinates and at the physical screen edge.
    Graphics::Shape red = {.type = Graphics::ShapeType::FillRectangle,
                           .fill_rectangle = {21, 33, 23, 35, Graphics::Colour::Red}};
    Graphics::Shape blue = {.type = Graphics::ShapeType::FillRectangle,
                            .fill_rectangle = {319, 221, 320, 222, Graphics::Colour::Blue}};
    Check(ui.GetScreen().SubmitShape(red) && ui.GetScreen().SubmitShape(blue),
          "Failed to submit partial updates");
    ui.Tick(0);
    Check(Pixel(display, 21, 33) == 0xF800 && Pixel(display, 22, 34) == 0xF800,
          "Partial update corrupted RGB565 palette conversion");
    Check(Pixel(display, 319, 221) == 0x001F, "The last display column did not update");
    const auto partial = Rows(display, 0, emulator::Ui::Height);
    ui.GetScreen().FullRefresh();
    ui.Tick(0);
    Check(Rows(display, 0, emulator::Ui::Height) == partial,
          "Dirty-tile output differs from a full framebuffer refresh");

    ui.Tick(1000);
    Check(Pixel(display, 1, 225) == 0, "The initial cursor should be hollow");
    ui.Tick(2000);
    Check(Pixel(display, 1, 225) == 0xFFFF, "The cursor did not blink using host ticks");

    ui.TextInput("Draft");
    ui.ChangeView(Renderer::View::MainMenu);
    Check(ui.GetScreen().UserTextLength() == 0, "Switching views must clear the draft");
    ui.Tick(2000);
    Check(Rows(display, 0, 20) != std::vector<uint16_t>(chat.begin(), chat.begin() + 20 * 320),
          "The firmware main menu did not change the title");
    display.Present();
}

void CheckScreenshot(const emulator::SdlDisplayDriver& display,
                     const std::string& path,
                     bool visible)
{
    display.SaveScreenshot(path);
    using Surface = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;
    Surface saved(SDL_LoadBMP(path.c_str()), SDL_FreeSurface);
    Check(saved != nullptr, "SDL could not load the emulator screenshot");
    Check(saved->w == 320 && saved->h == 240, "Screenshots must use the native display resolution");
    Surface rgba(SDL_ConvertSurfaceFormat(saved.get(), SDL_PIXELFORMAT_RGBA32, 0), SDL_FreeSurface);
    Check(rgba != nullptr, "Could not read screenshot pixels");
    size_t lit = 0;
    for (int y = 0; y < rgba->h; ++y)
    {
        const auto* row = static_cast<const uint8_t*>(rgba->pixels) + y * rgba->pitch;
        for (int x = 0; x < rgba->w; ++x)
        {
            lit += row[x * 4] != 0 || row[x * 4 + 1] != 0 || row[x * 4 + 2] != 0;
        }
    }
    Check(visible ? lit > 100 : lit == 0, "Screenshot does not reflect backlight/sleep state");
}

void TestPresentation(emulator::SdlDisplayDriver& display, const std::string& screenshot)
{
    const auto original = Rows(display, 0, emulator::Ui::Height);
    CheckScreenshot(display, screenshot, true);
    display.DisableBacklight();
    display.Present();
    CheckScreenshot(display, screenshot, false);
    display.EnableBacklight();
    CheckScreenshot(display, screenshot, true);
    display.Sleep();
    display.Present();
    CheckScreenshot(display, screenshot, false);
    display.Wake();
    display.Present();
    CheckScreenshot(display, screenshot, true);
    Check(Rows(display, 0, emulator::Ui::Height) == original,
          "Backlight/sleep must preserve display memory");
}
} // namespace

int main(int argc, char** argv)
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
    {
        std::cerr << SDL_GetError() << '\n';
        return 1;
    }
    int result = 0;
    try
    {
        emulator::SdlDisplayDriver display(emulator::Ui::Width, emulator::Ui::Height, 1, true);
        TestPixelStream(display);
        TestFirmwareViews(display);
        TestPresentation(display, argc > 1 ? argv[1] : "display-test.bmp");
        std::cout << "Display stream and shared firmware rendering checks passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    SDL_Quit();
    return result;
}

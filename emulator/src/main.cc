#include "sdl_display_driver.hh"
#include "ui.hh"
#include <SDL.h>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
constexpr const char* Usage =
    "Usage: hactar-ui [options]\n"
    "  --scale N           Initial window scale, 1..8 (default: 3)\n"
    "  --view VIEW         startup, chat, or menu (default: startup, then chat)\n"
    "  --headless          Use SDL's dummy video driver; no desktop required\n"
    "  --frames N          Exit after N frames\n"
    "  --screenshot PATH   Save a native-resolution BMP on exit\n"
    "  --help              Show this help\n"
    "\n"
    "Controls: F1 splash, F2 chat, F3 menu, F5 redraw, F8 backlight, Esc quit.\n"
    "In chat: type ASCII text, Backspace to edit, Enter for local echo.\n";

struct Options
{
    int scale = 3;
    uint32_t frames = 0;
    bool headless = false;
    bool help = false;
    std::optional<Renderer::View> view;
    std::string screenshot;
};

uint32_t ParseNumber(std::string_view value)
{
    uint32_t number = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || number == 0)
    {
        throw std::invalid_argument("Expected a positive integer: " + std::string(value));
    }
    return number;
}

Options ParseOptions(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];
        if (arg == "--help")
        {
            options.help = true;
        }
        else if (arg == "--headless")
        {
            options.headless = true;
        }
        else if (arg == "--scale" || arg == "--frames" || arg == "--view" || arg == "--screenshot")
        {
            if (++i >= argc)
            {
                throw std::invalid_argument("Missing value for " + std::string(arg));
            }
            const std::string_view value = argv[i];
            if (arg == "--scale")
            {
                const auto scale = ParseNumber(value);
                if (scale > 8)
                {
                    throw std::invalid_argument("Window scale must be in 1..8");
                }
                options.scale = static_cast<int>(scale);
            }
            else if (arg == "--frames")
            {
                options.frames = ParseNumber(value);
            }
            else if (arg == "--screenshot")
            {
                if (value.empty())
                {
                    throw std::invalid_argument("Screenshot path must not be empty");
                }
                options.screenshot = value;
            }
            else if (value == "startup")
            {
                options.view = Renderer::View::Startup;
            }
            else if (value == "chat")
            {
                options.view = Renderer::View::Chat;
            }
            else if (value == "menu")
            {
                options.view = Renderer::View::MainMenu;
            }
            else
            {
                throw std::invalid_argument("Unknown view: " + std::string(value));
            }
        }
        else
        {
            throw std::invalid_argument("Unknown option: " + std::string(arg));
        }
    }
    return options;
}

class SdlContext
{
public:
    explicit SdlContext(bool headless)
    {
        SDL_SetMainReady();
        if (headless && SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) != 0)
        {
            throw std::runtime_error("Failed to select SDL's dummy video driver");
        }
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        {
            throw std::runtime_error(std::string("Initialize SDL: ") + SDL_GetError());
        }
    }

    ~SdlContext()
    {
        SDL_Quit();
    }
};

int Run(const Options& options)
{
    SdlContext sdl(options.headless);
    emulator::SdlDisplayDriver display(emulator::Ui::Width, emulator::Ui::Height, options.scale,
                                       options.headless);
    emulator::Ui ui(display);
    if (options.view)
    {
        ui.ChangeView(*options.view);
    }
    bool auto_chat = !options.view.has_value();
    bool running = true;
    bool backlight = true;
    const uint32_t start = SDL_GetTicks();
    uint32_t frames = 0;
    SDL_StartTextInput();
    ui.Tick(0);

    while (running)
    {
        const uint32_t frame_start = SDL_GetTicks();
        const uint32_t ticks = frame_start - start;
        if (auto_chat && ticks >= 1000)
        {
            ui.ChangeView(Renderer::View::Chat);
            ui.Tick(ticks);
            auto_chat = false;
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_TEXTINPUT)
            {
                ui.TextInput(event.text.text);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    ui.CommitInput();
                    break;
                case SDLK_BACKSPACE:
                    ui.Backspace();
                    break;
                case SDLK_F1:
                case SDLK_F2:
                case SDLK_F3:
                    auto_chat = false;
                    ui.ChangeView(event.key.keysym.sym == SDLK_F1   ? Renderer::View::Startup
                                  : event.key.keysym.sym == SDLK_F2 ? Renderer::View::Chat
                                                                    : Renderer::View::MainMenu);
                    ui.Tick(ticks);
                    break;
                case SDLK_F5:
                    ui.GetScreen().FullRefresh();
                    break;
                case SDLK_F8:
                    backlight = !backlight;
                    if (backlight)
                    {
                        ui.GetScreen().EnableBacklight();
                    }
                    else
                    {
                        ui.GetScreen().DisableBacklight();
                    }
                    break;
                default:
                    break;
                }
            }
        }

        ui.Tick(ticks);
        display.Present();
        ++frames;
        if (options.frames != 0 && frames >= options.frames)
        {
            break;
        }
        const uint32_t elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < 16)
        {
            SDL_Delay(16 - elapsed);
        }
    }

    SDL_StopTextInput();
    if (!options.screenshot.empty())
    {
        display.SaveScreenshot(options.screenshot);
    }
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        const auto options = ParseOptions(argc, argv);
        if (options.help)
        {
            std::cout << Usage;
            return 0;
        }
        return Run(options);
    }
    catch (const std::exception& error)
    {
        std::cerr << "hactar-ui: " << error.what() << '\n';
        return 1;
    }
}

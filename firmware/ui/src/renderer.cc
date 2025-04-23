#include "renderer.hh"

Renderer::Renderer(Screen& screen, Keyboard& keyboard) :
    screen(screen),
    keyboard(keyboard),
    view(View::Startup),
    change_view(true)
{
}

void Renderer::Render(const uint32_t ticks) noexcept
{
    switch (view)
    {
    case View::Startup:
    {
        StartupView(ticks);
        break;
    }
    case View::Wifi:
    {
        WifiView(ticks);
        break;
    }
    case View::Rooms:
    {
        RoomsView(ticks);
        break;
    }
    case View::Chat:
    {
        RoomsView(ticks);
        break;
    }
    default:
    {
        Error("Renderer render", "Given view is not handled");
    }
    }
}

void Renderer::StartupView(const uint32_t ticks) noexcept
{
    if (change_view)
    {
        screen.FillScreen(Colour::Black);
        screen.UpdateTitle("Welcome to Hactar!", 18);
        change_view = false;
    }

    // Do other stuff.
    screen.Draw(ticks);
}

void Renderer::WifiView(const uint32_t ticks) noexcept
{
}

void Renderer::RoomsView(const uint32_t ticks) noexcept
{
}

void Renderer::ChatView(const uint32_t ticks) noexcept
{
}
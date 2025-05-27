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
    case View::Chat:
    {
        RoomsView(ticks);
        break;
    }
    case View::MainMenu:
    {
        MainMenuView(ticks);
        break;
    }
    case View::Rooms:
    {
        RoomsView(ticks);
        break;
    }
    case View::Wifi:
    {
        WifiView(ticks);
        break;
    }
    default:
    {
        Error("Renderer render", "Given view is not handled");
    }
    }

    // Do other stuff.
    screen.Draw(ticks);
}

void Renderer::ChangeView(const View view) noexcept
{
    this->view = view;
    change_view = true;
}

void Renderer::StartupView(const uint32_t ticks) noexcept
{
    if (change_view)
    {
        screen.FillScreen(Colour::Black);
        screen.UpdateTitle("Loading", 7);
        change_view = false;
    }
}

void Renderer::ChatView(const uint32_t ticks) noexcept
{
}

void Renderer::MainMenuView(const uint32_t ticks) noexcept
{
    if (change_view)
    {
        screen.FillScreen(Colour::Black);
        screen.UpdateTitle("Main menu", 9);

        screen.CommitText("1. Chat", 7);
        screen.CommitText("2. Wifi", 7);

        change_view = false;
    }
}

void Renderer::RoomsView(const uint32_t ticks) noexcept
{
}

void Renderer::WifiView(const uint32_t ticks) noexcept
{
}

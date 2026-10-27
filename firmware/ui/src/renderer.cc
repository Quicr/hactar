#include "renderer.hh"
#include "graphics.hh"

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
        ChatView(ticks);
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

    screen.AnimationUpdate(ticks);
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
        Refresh("Loading", 7, ticks);
    }
}

void Renderer::ChatView(const uint32_t ticks) noexcept
{
    if (change_view)
    {
        Refresh("Chat", 4, ticks);
    }
}

void Renderer::MainMenuView(const uint32_t ticks) noexcept
{
    if (change_view)
    {
        screen.FillScreen(Graphics::Colour::Black);
        screen.UpdateTitle("Main menu", 9);
        screen.DrawTitleLine();
        screen.DrawUserLine();

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

void Renderer::Refresh(const char* title, const size_t title_len, const uint32_t ticks) noexcept
{
    screen.FillScreen(Graphics::Colour::Black);
    screen.UpdateTitle(title, title_len);
    screen.DrawTitleLine();
    screen.DrawUserLine();
    screen.DrawIcon(Screen::IconType::Battery, false);
    screen.DrawIcon(Screen::IconType::Wifi, false);
    screen.DrawIcon(Screen::IconType::Moq, false);
    screen.DrawIcon(Screen::IconType::Speaker, false);
    screen.DrawIcon(Screen::IconType::Mic, false);
    screen.BatteryLevel(0);
    change_view = false;
}

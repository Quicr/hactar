#pragma once

#include "keyboard.hh"
#include "screen.hh"

class Renderer
{
public:
    typedef void (*DrawRoutine)(const uint32_t);

    enum class View : uint8_t
    {
        Startup = 0,
        Chat,
        MainMenu,
        Rooms,
        Wifi,
    };

    enum class MenuItem : uint8_t
    {
        Chat,
        Wifi
    };

    Renderer(Screen& screen, Keyboard& keyboard);

    void SetDrawFunction(DrawRoutine draw_routine);

    void Render(const uint32_t ticks) noexcept;

    void ChangeView(const View view) noexcept;

private:
    void StartupView(const uint32_t ticks) noexcept;
    void ChatView(const uint32_t ticks) noexcept;
    void MainMenuView(const uint32_t ticks) noexcept;
    void RoomsView(const uint32_t ticks) noexcept;
    void WifiView(const uint32_t ticks) noexcept;

    // static void DefaultDraw(const uint32_t ticks);
private:
    void Refresh(const char* title, const size_t title_len, const uint32_t ticks) noexcept;

    Screen& screen;
    Keyboard& keyboard;
    View view;

    bool change_view;
};

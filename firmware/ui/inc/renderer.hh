#pragma once

#include "keyboard.hh"
#include "screen.hh"

class Renderer
{
public:
    enum class View : uint8_t
    {
        Startup = 0,
        Wifi,
        Rooms,
        Chat
    };

    Renderer(Screen& screen, Keyboard& keyboard);

    void Render(const uint32_t ticks) noexcept;

private:
    void StartupView(const uint32_t ticks) noexcept;
    void WifiView(const uint32_t ticks) noexcept;
    void RoomsView(const uint32_t ticks) noexcept;
    void ChatView(const uint32_t ticks) noexcept;

    void Refresh(const uint32_t ticks) noexcept;

    Screen& screen;
    Keyboard& keyboard;
    View view;

    bool change_view;
};
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

    void Refresh(const uint32_t ticks) noexcept;

    // static void DefaultDraw(const uint32_t ticks);
private:
    static constexpr uint32_t User_Input_Buff_Size = Screen::Max_Characters;

    Screen& screen;
    Keyboard& keyboard;
    View view;

    bool change_view;

    char user_input[User_Input_Buff_Size] = {0};
    uint32_t user_input_idx = 0;
};
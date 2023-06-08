#include "WifiView.hh"

#include "WifiView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

WifiView::WifiView(UserInterfaceManager& manager,
                   Screen& screen,
                   Q10Keyboard& keyboard,
                   SettingManager& setting_manager)
    : ViewInterface(manager, screen, keyboard, setting_manager)
{
}

WifiView::~WifiView()
{

}

void WifiView::Update()
{
    // Periodically get the list of teams.
    return;
}

void WifiView::AnimatedDraw()
{

}

void WifiView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            // screen.FillRectangle(0, 13, screen.ViewWidth(), 14, fg);
            screen.DrawText(0, 1, "Wifi settings", menu_font, fg, bg);
            first_load = false;
        }
    }

    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        DrawInputString(draw_str);
    }
}

void WifiView::HandleInput()
{
    // Parse commands
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);
    }
    else
    {

    }
}

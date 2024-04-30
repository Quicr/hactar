#include "TeamView.hh"
#include "UserInterfaceManager.hh"

TeamView::TeamView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager,
    Network& network)
    : ViewInterface(manager, screen, keyboard, setting_manager, network)
{
}

TeamView::~TeamView()
{

}

void TeamView::Update()
{
    // Periodically get the list of teams.
    return;
}

void TeamView::AnimatedDraw()
{

}

void TeamView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            // screen.FillRectangle(0, 13, screen.ViewWidth(), 14, fg);
            screen.DrawText(0, 1, "Teams", menu_font, fg, bg);
            first_load = false;
        }
    }

    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        std::string draw_str;
        draw_str = usr_input.substr(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        DrawInputString(draw_str);
    }
}

void TeamView::HandleInput()
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

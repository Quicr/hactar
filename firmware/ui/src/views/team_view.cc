#include "team_view.hh"
#include "user_interface_manager.hh"

TeamView::TeamView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio)
    : ViewInterface(manager, screen, keyboard, setting_manager, serial, network, audio)
{
}

TeamView::~TeamView()
{

}

void TeamView::Update(uint32_t current_tick)
{
    UNUSED(current_tick);
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
            screen.DrawStringAsync(0, 1, "Teams", menu_font, fg, bg, false);
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

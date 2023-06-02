#include "TeamView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

TeamView::TeamView(UserInterfaceManager& manager,
                   Screen& screen,
                   Q10Keyboard& keyboard,
                   SettingManager& setting_manager)
    : ViewInterface(manager, screen, keyboard, setting_manager)
{
}

TeamView::~TeamView()
{

}

bool TeamView::Update()
{
    // Periodically get the list of teams.
    return false;
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
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        DrawInputString(draw_str);
    }
}

bool TeamView::HandleInput()
{
    // Handle the input from the user
    // If they enter a command for going to settings then change views
    GetInput();
    if (!keyboard.EnterPressed()) return false;

    if (!(usr_input.length() > 0)) return false;

    // Parse commands
    if (usr_input[0] == '/')
    {
        String command = usr_input.substring(1);

        command_handler->ChangeViewCommand(command);
    }
    else
    {
        //


        // Send message to sec layer
    }

    ClearInput();
    return false;
}

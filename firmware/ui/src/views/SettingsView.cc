#include "SettingsView.hh"
#include "ChatView.hh"
#include "UserInterfaceManager.hh"

SettingsView::SettingsView(UserInterfaceManager& manager,
                           Screen& screen,
                           Q10Keyboard& keyboard,
                           SettingManager& setting_manager)
    : ViewBase(manager, screen, keyboard, setting_manager)
{
}

SettingsView::~SettingsView()
{

}

void SettingsView::AnimatedDraw()
{

}

void SettingsView::Draw()
{
    ViewBase::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            uint16_t speed = 0;
            String msg = "Settings";
            screen.DrawBlockAnimateString(0, 6, msg, font11x16, fg, bg, speed);
            first_load = false;
        }
    }

    if ((usr_input.length() > last_drawn_idx || redraw_input))
    {
        String draw_str;
        // Fill the draw string buffer with stars instead.
        while (last_drawn_idx < usr_input.length())
        {

            if (usr_input.length() > last_drawn_idx || redraw_input)
            {
                // Shift over and draw the input that is currently in the buffer
                String draw_str;
                draw_str = usr_input.substring(last_drawn_idx);
                last_drawn_idx = usr_input.length();
                DrawInputString(draw_str);
            }
        }

        DrawInputString(draw_str);
    }
}

bool SettingsView::HandleInput()
{
    // Handle the input from the user, if they get the correct passcode
    // Change to the chat view;
    GetInput();

    if (!keyboard.EnterPressed()) return false;
    if (!(usr_input.length() > 0)) return false;

    // Check if this is a command
    if (usr_input[0] == '/')
    {
        String command = usr_input.substring(1);

        command_handler->ChangeViewCommand(command);
    }

    ClearInput();

    return false;
}

bool SettingsView::Update()
{
    return false;
}


#include "settings_view.hh"
#include "user_interface_manager.hh"

SettingsView::SettingsView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio)
    : ViewInterface(manager, screen, keyboard, setting_manager, serial, network, audio)
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
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            uint16_t speed = 0;
            std::string msg = "Settings";
            screen.DrawBlockAnimateString(0, 6, msg, font11x16, fg, bg, speed);
            first_load = false;
        }
    }

    if ((usr_input.length() > last_drawn_idx || redraw_input))
    {
        std::string draw_str;
        // Fill the draw string buffer with stars instead.
        while (last_drawn_idx < usr_input.length())
        {

            if (usr_input.length() > last_drawn_idx || redraw_input)
            {
                // Shift over and draw the input that is currently in the buffer
                std::string draw_str;
                draw_str = usr_input.substr(last_drawn_idx);
                last_drawn_idx = usr_input.length();
                DrawInputString(draw_str);
            }
        }

        DrawInputString(draw_str);
    }
}

void SettingsView::HandleInput()
{
    // Handle the input from the user, if they get the correct passcode
    // Change to the chat view;
    GetInput();

    if (!keyboard.EnterPressed()) return;
    if (!(usr_input.length() > 0)) return;

    // Check if this is a command
    if (usr_input[0] == '/')
    {
        std::string command = usr_input.substr(1);

        // command_handler->ChangeViewCommand(command);
    }

    ClearInput();
}

void SettingsView::Update(uint32_t current_tick)
{
    return;
}


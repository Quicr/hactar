#include "login_view.hh"
#include "user_interface_manager.hh"

LoginView::LoginView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio)
    : ViewInterface(manager, screen, keyboard, setting_manager, serial, network, audio),
    incorrect_passcode_entered(false)
{
}

LoginView::~LoginView()
{

}

void LoginView::AnimatedDraw()
{
    if (!redraw_menu)
    {
        return;
    }

    if (manager.RedrawForced())
    {
        return;
    }

    if (!first_load)
    {
        return;
    }

    uint16_t speed = 10;
    const uint16_t y = 16;
    std::string msg = "Welcome to Cisco";
    screen.DrawStringAsync(
        screen.GetStringCenterMargin(msg.length(), font11x16), y, msg,
        font11x16, fg, bg, speed);
    msg = "Secure Messaging";
    screen.DrawStringAsync(
        screen.GetStringCenterMargin(msg.length(), font11x16),
        y+font11x16.height, msg,
        font11x16, fg, bg, speed);

    msg = "User: ";
    screen.DrawStringAsync(1, screen.ViewHeight() - (usr_font.height * 4),
        msg, usr_font, fg, bg, speed);

    screen.DrawStringAsync(
        1 + usr_font.width * msg.length(), screen.ViewHeight() - (usr_font.height * 4),
        setting_manager.Username()->c_str(), usr_font, fg, bg, speed);

    msg = "Enter your passcode";
    screen.DrawStringAsync(1, screen.ViewHeight() - (usr_font.height * 2),
        msg, usr_font, fg, bg, speed);



    first_load = false;
}

void LoginView::Draw()
{
    ViewInterface::Draw();

    if ((usr_input.length() > last_drawn_idx || redraw_input))
    {
        std::string draw_str;
        // Fill the draw string buffer with stars instead.
        while (last_drawn_idx < usr_input.length())
        {
            draw_str.push_back('*');
            last_drawn_idx++;
        }

        ViewInterface::DrawInputString(draw_str);
    }

    if (!redraw_menu)
        return;

    if (incorrect_passcode_entered)
    {
        std::string msg = "** Incorrect passcode **";
        screen.DrawStringAsync(1,
            screen.ViewHeight() - (usr_font.height * 3),
            msg, usr_font,
            fg, bg, false);
    }
}

void LoginView::Update(uint32_t current_tick)
{

}

void LoginView::HandleInput()
{
    // Handle the input from the user, if they get the correct passcode
    // Change to the chat view;

    // TODO remove this is bad!
    char* eeprom_password = nullptr;
    short eeprom_password_len = 0;
    setting_manager.LoadSetting(SettingManager::SettingAddress::Password,
        &eeprom_password, eeprom_password_len);
    std::string password;
    for (short i = 0 ; i < eeprom_password_len; ++i)
    {
        password.push_back(eeprom_password[i]);
    }
    delete eeprom_password;
    // Ugly code above...

    if (usr_input == password)
    {
        ChangeView("/rooms");
    }
    else
    {
        std::string msg = "** Incorrect passcode **";
        screen.DrawStringAsync(1,
            screen.ViewHeight() - (usr_font.height * 3), msg, usr_font,
            fg, bg, false);

    }
}
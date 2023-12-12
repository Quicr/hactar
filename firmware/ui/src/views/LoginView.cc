#include "LoginView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

LoginView::LoginView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager) :
    ViewInterface(manager, screen, keyboard, setting_manager),
    incorrect_passcode_entered(false)
{
}

LoginView::~LoginView()
{

}

void LoginView::AnimatedDraw()
{
    if (!redraw_menu)
        return;

    if (manager.RedrawForced())
        return;

    if (!first_load)
        return;

    uint16_t speed = 10;
    String msg = "Welcome to Cisco";
    screen.DrawBlockAnimateString(
        screen.GetStringCenterMargin(msg.length(), font11x16), 6, msg,
        font11x16, fg, bg, speed);
    msg = "Secure Messaging";
    screen.DrawBlockAnimateString(
        screen.GetStringCenterMargin(msg.length(), font11x16), 22, msg,
        font11x16, fg, bg, speed);

    // TODO

    msg = "User: ";
    screen.DrawBlockAnimateString(1, screen.ViewHeight() - (usr_font.height * 4),
        msg, usr_font, fg, bg, speed);

    screen.DrawBlockAnimateString(
        1 + usr_font.width * msg.length(), screen.ViewHeight() - (usr_font.height * 4),
        manager.GetUsername(), usr_font, fg, bg, speed);

    msg = "Enter your passcode";
    screen.DrawBlockAnimateString(1, screen.ViewHeight() - (usr_font.height * 2),
        msg, usr_font, fg, bg, speed);

    first_load = false;
}

void LoginView::Draw()
{
    ViewInterface::Draw();

    if ((usr_input.length() > last_drawn_idx || redraw_input))
    {
        String draw_str;
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
        String msg = "** Incorrect passcode **";
        screen.DrawText(1,
            screen.ViewHeight() - (usr_font.height * 3), msg, usr_font,
            fg, bg);
    }
}

void LoginView::Update()
{

}

void LoginView::HandleInput()
{
    // Handle the input from the user, if they get the correct passcode
    // Change to the chat view;

    // TODO remove this is bad!
    char* eeprom_password;
    short eeprom_password_len;
    setting_manager.LoadSetting(SettingManager::SettingAddress::Password,
        &eeprom_password, eeprom_password_len);
    String password;
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
        String msg = "** Incorrect passcode **";
        screen.DrawText(1,
            screen.ViewHeight() - (usr_font.height * 3), msg, usr_font,
            fg, bg);

    }
}
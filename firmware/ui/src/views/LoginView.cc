#include "LoginView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

LoginView::LoginView(UserInterfaceManager& manager,
                     Screen& screen,
                     Q10Keyboard& keyboard,
                     SettingManager& setting_manager) :
    ViewBase(manager, screen, keyboard, setting_manager),
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
    String msg = "Welcome ";
    screen.DrawBlockAnimateString(34, 6, msg, font11x16, fg, bg, speed);
    // msg = "Secure Messaging";

    char* username;
    unsigned char len;
    setting_manager.LoadSetting(SettingManager::SettingAddress::Username,
        username, len);
    screen.DrawBlockAnimateString(34, 22, msg, font11x16, fg, bg, speed);
    msg = "Enter your passcode";
    screen.DrawBlockAnimateString(1,
        screen.ViewHeight() - (usr_font.height * 2), msg, usr_font, fg,
        bg, speed);

    first_load = false;
}

void LoginView::Draw()
{
    ViewBase::Draw();

    if ((usr_input.length() > last_drawn_idx || redraw_input))
    {
        String draw_str;
        // Fill the draw string buffer with stars instead.
        while (last_drawn_idx < usr_input.length())
        {
            draw_str.push_back('*');
            last_drawn_idx++;
        }

        DrawInputString(draw_str);
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

bool LoginView::HandleInput()
{
    // Handle the input from the user, if they get the correct passcode
    // Change to the chat view;
    GetInput();

    if (!keyboard.EnterPressed()) return false;

    if (usr_input == passcode)
    {
        manager.ChangeView<ChatView>();
        return true;
    }
    else
    {
        String msg = "** Incorrect passcode **";
        screen.DrawText(1,
            screen.ViewHeight() - (usr_font.height * 3), msg, usr_font,
            fg, bg);

    }


    ClearInput();

    return false;
}
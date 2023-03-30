#include "FirstBootView.hh"

FirstBootView::FirstBootView(UserInterfaceManager& manager,
                             Screen& screen,
                             Q10Keyboard& keyboard,
                             EEPROM& eeprom) :
    ViewBase(manager, screen, keyboard, eeprom),
    state(State::Username)
{
    // Clear the whole eeprom
    eeprom.Clear();
    eeprom.Write((unsigned char)FIRST_BOOT_STARTED);
}

FirstBootView::~FirstBootView()
{

}

void FirstBootView::AnimatedDraw()
{
    if (!redraw_menu)
        return;

    if (manager.RedrawForced())
        return;

    if (!first_load)
        return;

    uint16_t speed = 10;
    String msg = "Welcome to Cisco";
    screen.DrawBlockAnimateString(34, 6, msg, font11x16, fg, bg, speed);
    msg = "Secure Messaging";
    screen.DrawBlockAnimateString(34, 22, msg, font11x16, fg, bg, speed);

    first_load = false;
}

void FirstBootView::Draw()
{
    ViewBase::Draw();

    String message;
    switch (state)
    {
        case State::Username:
        {
            message = "Please enter your name:";
            break;
        }
        case State::Passcode:
        {
            message = "Please enter a passcode:";
            break;
        }
        case State::Wifi:
        {
            // TODO get wifi's that are in range, and have them type a number
            // then ask for a password
            // two stage wifi..
            break;
        }
        case State::Final:
        {
            message = "Thank you for completing the first boot";

            // Set the view address to 0x02
            eeprom.Write((unsigned char*)FIRST_BOOT_DONE);

            HAL_Delay(1000);
            break;
        }
    }

    screen.DrawText(1,
        screen.ViewHeight() - (usr_font.height * 2), message, usr_font, fg, bg);

    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        DrawInputString(draw_str);
    }
}

bool FirstBootView::HandleInput()
{
    // Handle the input from the user.
    GetInput();

    if (!keyboard.EnterPressed()) return false;

    // TODO switch statement
    switch (state)
    {
        case State::Username:
        {
            // Write the data
            manager.UsernameAddr() = eeprom.Write(usr_input.data(),
                usr_input.length());

            state = State::Passcode;
            break;
        }
        case State::Passcode:
        {
            // TODO save address
            // Write the data
            manager.PasscodeAddr() = eeprom.Write(usr_input.data(),
                usr_input.length());

            state = State::Wifi;
            break;
        }
        case State::Wifi:
        {
            SetWifi();
            break;
        }
        case State::Final:
        {
            SetFinal();
            break;
        }
        default:
        {
            state = State::Username;
        }
    }


    // Which is 0
    ClearInput();
    return false;
}

void FirstBootView::SetUsername()
{
    // Write the data
    unsigned int address = eeprom.Write(usr_input.data(), usr_input.length());
    state = State::Passcode;
}

void FirstBootView::SetPasscode()
{

}

void FirstBootView::SetWifi()
{

}

void FirstBootView::SetFinal()
{

}

void FirstBootView::SetAllDefaults()
{

}
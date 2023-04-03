#include "FirstBootView.hh"

FirstBootView::FirstBootView(UserInterfaceManager& manager,
                             Screen& screen,
                             Q10Keyboard& keyboard,
                             SettingManager& setting_manager) :
    ViewBase(manager, screen, keyboard, setting_manager),
    state(State::Username),
    request_message("Please enter your name:")
{
    // Clear the whole eeprom
    setting_manager.ClearEeprom();
    setting_manager.SaveSetting(SettingManager::SettingManager::Firstboot,
        (uint8_t)FIRST_BOOT_STARTED);
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

    screen.DrawText(1,
        screen.ViewHeight() - (usr_font.height * 2), request_message,
            usr_font, fg, bg);

    auto ssids = manager.SSIDs();
    uint16_t idx = 0;
    for (auto ssid : ssids)
    {
        uint16_t len = ssid.second.length();
        const char* message = ssid.second.c_str();
        const uint16_t y_start = 50;
        screen.DrawText(1, y_start + (idx * usr_font.height),
            String::int_to_string(ssid.first), usr_font, C_WHITE, C_BLACK);
        screen.DrawText(1 + usr_font.width * 3, y_start + (idx * usr_font.height),
            ssid.second, usr_font, C_WHITE, C_BLACK);
        ++idx;
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

bool FirstBootView::HandleInput()
{
    // Handle the input from the user.
    GetInput();

    if (!keyboard.EnterPressed()) return false;

    // TODO error checking
    switch (state)
    {
        case State::Username:
        {
            // Write the data
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::Username,
                usr_input.data(), usr_input.length());

            request_message = "Please enter a passcode:";
            state = State::Passcode;
            break;
        }
        case State::Passcode:
        {
            // TODO need to hide the passcode as they type it?
            // Write the data
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::Password,
                usr_input.data(), usr_input.length());

            request_message = "Please select SSID by number:";

            Packet ssid_req_packet;
            ssid_req_packet.SetData(Packet::Types::Command, 0, 6);
            ssid_req_packet.SetData(manager.NextPacketId(), 6, 8);
            ssid_req_packet.SetData(1, 14, 10);
            ssid_req_packet.SetData(Packet::Commands::SSIDs, 24, 8);
            manager.EnqueuePacket(std::move(ssid_req_packet));
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

    ClearInput();
    return false;
}

void FirstBootView::SetWifi()
{
    if (manager.SSIDAddr() == 0)
    {
        // Get the ssid selection
        uint32_t ssid = usr_input.ToNumber();
        if (ssid == -1)
        {
            request_message = "Error. Please select SSID by number:";
            return;
        }

        const std::map<uint8_t, String>& ssids = manager.SSIDs();

        if (ssids.find(ssid) == ssids.end())
        {
            request_message = "Error. Please select SSID number:";
            return;
        }

        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID,
            usr_input.data(), usr_input.length());
    }
    else if (manager.SSIDPasscodeAddr() == 0)
    {
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID_Password,
            usr_input.data(), usr_input.length());
        // TODO Wait until a connection is returned
    }
}

void FirstBootView::SetFinal()
{

}

void FirstBootView::SetAllDefaults()
{

}
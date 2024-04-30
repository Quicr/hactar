#include "FirstBootView.hh"
#include "LoginView.hh"
#include "SerialPacket.hh"
#include "fonts/Font.hh"

// TODO make sure the usr input is not empty

FirstBootView::FirstBootView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager,
    Network& network)
    : ViewInterface(manager, screen, keyboard, setting_manager, network),
    state(State::Username),
    request_message("Please enter your name:"),
    wifi_state(WifiState::SSID),
    ssids(nullptr),
    ssid(),
    password(),
    state_update_timeout(0),
    num_connection_checks(0)
{
    // Clear the whole eeprom
    setting_manager.ClearEeprom();
    setting_manager.SaveSetting(SettingManager::SettingAddress::Firstboot,
        (uint8_t)FIRST_BOOT_STARTED);
    setting_manager.SaveSetting(SettingManager::SettingAddress::Usr_Font,
        (uint8_t)Fonts::_7x12);
    setting_manager.SaveSetting(SettingManager::SettingAddress::Menu_Font,
        (uint8_t)Fonts::_11x16);
    setting_manager.SaveSetting(SettingManager::SettingAddress::Fg,
        (uint16_t)C_WHITE);
    setting_manager.SaveSetting(SettingManager::SettingAddress::Bg,
        (uint16_t)C_BLACK);
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
    std::string msg = "Welcome to Cisco";
    screen.DrawBlockAnimateString(
        screen.GetStringCenterMargin(msg.length(), font11x16), 6,
        msg, font11x16, fg, bg, speed);
    msg = "Secure Messaging";
    screen.DrawBlockAnimateString(
        screen.GetStringCenterMargin(msg.length(), font11x16), 22,
        msg, font11x16, fg, bg, speed);

    first_load = false;
}

void FirstBootView::Draw()
{
    ViewInterface::Draw();

    screen.DrawText(1,
        screen.ViewHeight() - (usr_font.height * 2), request_message,
        usr_font, fg, bg);

    if (wifi_state == WifiState::SSID)
    {
        DrawSSIDs();
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

void FirstBootView::HandleInput()
{
    // Handle the input from the user.

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

            network.RequestSSIDs();

            state = State::Wifi;
            break;
        }
        case State::Wifi:
        {
            SetWifi();
            break;
        }
        default:
        {
            state = State::Username;
        }
    }
}

void FirstBootView::Update()
{
    if (state == State::Wifi)
    {
        ssids = &network.GetSSIDs();

        if (wifi_state == WifiState::Connecting)
        {
            UpdateConnecting();
        }
    }
    else if (state == State::Final)
    {
        // Save to the eeprom that we finished the first boot
        if (HAL_GetTick() > state_update_timeout)
        {
            ChangeView("/login");
        }
    }
}

void FirstBootView::SetWifi()
{
    if (wifi_state == WifiState::SSID)
    {
        // Get the ssid selection
        if (usr_input == "skip")
        {
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::SSID,
                "1", 1);
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::SSID_Password,
                "1", 1);

            state = State::Final;
            return;
        }


        // Nothing to do with empty ssids
        if (ssids == nullptr)
        {
            return;
        }

        // My String class had a built in ToNumber that doesn't had exceptions
        // but instead would return a -1...
        // Read the value that was entered by the user
        // int32_t room_id = usr_input.ToNumber();
        // To avoid exceptions for now
        char* str_part;
        int32_t ssid_id = strtol(usr_input.c_str(), &str_part, 10);

        // May never get called now..
        if (ssid_id == -1)
        {
            request_message = "Error. Please select SSID number:";
            return;
        }

        if (ssids->find(ssid_id) == ssids->end())
        {
            request_message = "Error. Please select SSID number:";
            return;
        }

        ssid = ssids->at(ssid_id);
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID,
            ssid.data(), ssid.length());

        // THINK should this be moved somewhere else?
        // Clear the area above the user input
        const uint16_t y_start = 50;
        screen.FillRectangle(1, y_start, WIDTH, HEIGHT, C_BLACK, 32);

        request_message = "Please enter the wifi password";
        wifi_state = WifiState::Password;
    }
    else if (wifi_state == WifiState::Password)
    {
        password = usr_input;
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID_Password,
            password.data(), password.length());

        network.ConnectToNetwork();

        // Set the state to waiting
        request_message = "Connecting";

        // TODO consider making a draw queue
        screen.FillRectangle(0, HEIGHT - usr_font.height * 2, WIDTH,
            HEIGHT - usr_font.height, C_BLACK);

        wifi_state = WifiState::Connecting;
    }
}

void FirstBootView::UpdateConnecting()
{
    if (HAL_GetTick() < state_update_timeout) return;

    if (manager.IsConnectedToWifi())
    {
        request_message = "Device setup successful";

        setting_manager.SaveSetting(SettingManager::SettingAddress::Firstboot,
            (uint8_t)FIRST_BOOT_DONE);

        state_update_timeout = HAL_GetTick() + 5000;
        state = State::Final;
        return;
    }

    if (++num_connection_checks > 30)
    {
        // Failed to connect to the internet repeat connecting to ssid
        wifi_state = WifiState::SSID;
        request_message = "Failed to connect. Select SSID";
        num_connection_checks = 0;
        return;
    }

    request_message += ".";

    if (request_message.length() > 13)
    {
        request_message = "Connecting";
        const uint16_t y_start = 50;
        screen.FillRectangle(request_message.length() * usr_font.width, y_start,
            WIDTH, HEIGHT, C_BLACK, 32);
    }
    state_update_timeout = HAL_GetTick() + 1000;
}

void FirstBootView::SetAllDefaults()
{

}

void FirstBootView::DrawSSIDs()
{
    uint16_t idx = 0;

    if (ssids == nullptr)
    {
        return;
    }

    for (auto ssid : *ssids)
    {
        const uint16_t y_start = 50;

        // convert the ssid int val to a string
        const std::string ssid_id_str = std::to_string(ssid.first);

        screen.FillRectangle(1 + usr_font.width * ssid_id_str.length(),
            y_start + (idx * usr_font.height),
            1 + usr_font.width * 3,
            y_start + ((idx + 1) * usr_font.height), C_BLACK);

        screen.DrawText(1, y_start + (idx * usr_font.height),
            ssid_id_str, usr_font, C_WHITE, C_BLACK);

        screen.DrawText(1 + usr_font.width * 3, y_start + (idx * usr_font.height),
            ssid.second, usr_font, C_WHITE, C_BLACK);
        ++idx;
    }
}

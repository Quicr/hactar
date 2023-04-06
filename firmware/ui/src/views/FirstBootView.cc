#include "FirstBootView.hh"
#include "LoginView.hh"

// TODO make sure the usr input is not empty

FirstBootView::FirstBootView(UserInterfaceManager& manager,
                             Screen& screen,
                             Q10Keyboard& keyboard,
                             SettingManager& setting_manager) :
    ViewBase(manager, screen, keyboard, setting_manager),
    state(State::Username),
    request_message("Please enter your name:"),
    wifi_state(WifiState::SSID),
    ssid(),
    password(),
    state_update_timeout(0),
    num_connection_checks(0)
{
    // Clear the whole eeprom
    setting_manager.ClearEeprom();
    setting_manager.SaveSetting(SettingManager::SettingAddress::Firstboot,
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

    if (wifi_state == WifiState::SSID)
    {
        auto ssids = manager.SSIDs();

        // TODO move into function
        uint16_t idx = 0;
        for (auto ssid : ssids)
        {
            uint16_t len = ssid.second.length();
            const char* message = ssid.second.c_str();
            const uint16_t y_start = 50;

            const String ssid_id_str = String::int_to_string(ssid.first);

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
        default:
        {
            state = State::Username;
        }
    }

    ClearInput();
    return false;
}

void FirstBootView::Update()
{
    // Oh boy
    if (state == State::Wifi)
    {
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
            // FIX sometimes crashes here
            // because we need to return true all the way upwards
            manager.ChangeView<LoginView>();
            return;
        }
    }

}

void FirstBootView::SetWifi()
{
    if (wifi_state == SSID)
    {
        // Get the ssid selection
        uint32_t ssid_id = usr_input.ToNumber();
        if (ssid_id == -1)
        {
            request_message = "Error. Please select SSID number:";
            return;
        }

        const std::map<uint8_t, String>& ssids = manager.SSIDs();

        if (ssids.find(ssid_id) == ssids.end())
        {
            request_message = "Error. Please select SSID number:";
            return;
        }

        ssid = ssids.at(ssid_id);
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
        password = "Spikeball21Computers";
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID_Password,
            password.data(), password.length());
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID_Password,
            password.data(), password.length());

        // Send a wifi connection packet to esp
        Packet connect_packet;
        connect_packet.SetData(Packet::Types::Command, 0, 6);
        connect_packet.SetData(manager.NextPacketId(), 6, 8);
        // THINK should these be separate packets?
        // +3 for the length of the ssid, length of the password
        uint16_t length = ssid.length() + password.length() + 4;
        connect_packet.SetData(ssid.length() + password.length() + 3, 14, 10);

        connect_packet.SetData(Packet::Commands::ConnectToSSID, 24, 8);

        // Set the length of the ssid
        connect_packet.SetData(ssid.length(), 32, 8);

        // Populate with the ssid
        uint16_t i;
        uint16_t offset = 40;
        for (i = 0; i < ssid.length(); ++i)
        {
            connect_packet.SetData(ssid[i], offset, 8);
            offset += 8;
        }

        // Set the length of the password
        connect_packet.SetData(password.length(), offset, 8);
        offset += 8;

        // Populate with the password
        uint16_t j;
        for (j = 0; j < password.length(); ++j)
        {
            char ch = password[j];
            connect_packet.SetData(password[j], offset, 8);
            offset += 8;
        }

        // Enqueue the message
        manager.EnqueuePacket(std::move(connect_packet));

        // Set the state to waiting
        request_message = "Connecting";

        // TODO consider making a draw queue
        screen.FillRectangle(0, HEIGHT - usr_font.height*2, WIDTH,
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
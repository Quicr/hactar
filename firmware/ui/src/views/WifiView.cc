#include "WifiView.hh"

#include "WifiView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

#include "main.hh"

WifiView::WifiView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager)
    : ViewInterface(manager, screen, keyboard, setting_manager),
    last_num_ssids(0),
    next_get_ssid_timeout(0),
    state(SSID),
    request_msg("Select an ssid number:"),
    ssid(),
    password(),
    state_update_timeout(0),
    num_connection_checks(0),
    connecting_done(false)
{
}

WifiView::~WifiView()
{

}

void WifiView::Update()
{
    uint32_t current_tick = HAL_GetTick();
    // Periodically get the list of teams.
    if (current_tick > next_get_ssid_timeout && state == WifiState::SSID)
    {
        ssids.clear();
        SendGetSSIDPacket();

        // Get the list again after 30 seconds
        next_get_ssid_timeout = current_tick + 30000;
    }

    if (state == WifiState::SSID && current_tick > state_update_timeout)
    {
        RingBuffer<std::unique_ptr<Packet>>* ssid_packets;

        if (manager.GetReadyPackets(&ssid_packets, Packet::Commands::SSIDs))
        {
            while (ssid_packets->Unread() > 0)
            {
                auto rx_packet = std::move(ssid_packets->Read());
                // Get the packet len
                uint16_t len = rx_packet->GetData(14, 10);

                // Get the ssid id
                uint8_t ssid_id = rx_packet->GetData(32, 8);

                // Build the string
                String str;
                for (uint8_t i = 0; i < len - 2; ++i)
                {
                    str.push_back(static_cast<char>(
                        rx_packet->GetData(40 + i * 8, 8)));
                }

                ssids[ssid_id] = std::move(str);
            }
        }

        state_update_timeout = current_tick + 500;
    }
    else if (state == WifiState::Connecting && current_tick > state_update_timeout)
    {
        if (connecting_done)
        {
            state = WifiState::SSID;
            request_msg = "Select an ssid number:";
            connecting_done = false;
            redraw_menu = true;
            return;
        }

        if (manager.IsConnectedToWifi())
        {
            request_msg = "Connected to ";
            request_msg += ssid;
            num_connection_checks = 0;

            // Save the new ssid and password
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::SSID,
                ssid.data(), ssid.length());
            setting_manager.SaveSetting(
                SettingManager::SettingAddress::SSID_Password,
                password.data(), password.length());

            redraw_menu = true;
            connecting_done = true;
        }

        if (++num_connection_checks > 30)
        {
            // Failed to connect to the internet repeat connecting to ssid
            state = WifiState::SSID;
            request_msg = "Failed to connect. Select SSID";
            num_connection_checks = 0;
            redraw_menu = true;
            return;
        }

        request_msg += ".";

        if (request_msg.length() > 13)
        {
            request_msg = "Connecting";
        }
        redraw_menu = true;
        state_update_timeout = current_tick + 1000;
    }
}

void WifiView::AnimatedDraw()
{

}

void WifiView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            // screen.FillRectangle(0, 13, screen.ViewWidth(), 14, fg);
            screen.DrawText(0, 1, "Wifi settings", menu_font, fg, bg);
            first_load = false;
        }

        screen.FillRectangle(1,
            screen.ViewHeight() - (usr_font.height * 3), screen.ViewWidth(), screen.ViewHeight() - (usr_font.height * 2), bg);
        screen.DrawText(1, screen.ViewHeight() - (usr_font.height * 3),
            request_msg, usr_font, fg, bg);
        redraw_menu = false;
    }

    if (state == WifiState::SSID && last_num_ssids != ssids.size())
    {
        const uint16_t y_start = 50;
        // Clear the area for wifi
        screen.FillRectangle(0, y_start, screen.ViewWidth(),
            screen.ViewHeight() - 100,
            C_BLACK);

        uint16_t idx = 0;
        for (auto ssid : ssids)
        {

            // convert the ssid int val to a string
            const String ssid_id_str = String::int_to_string(ssid.first);

            screen.DrawText(1, y_start + (idx * usr_font.height),
                ssid_id_str, usr_font, C_WHITE, C_BLACK);

            screen.DrawText(1 + usr_font.width * 3, y_start + (idx * usr_font.height),
                ssid.second, usr_font, C_WHITE, C_BLACK);
            ++idx;
        }

        last_num_ssids = ssids.size();
    }

    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        String draw_str;
        if (state != WifiState::Password)
        {
            // Shift over and draw the input that is currently in the buffer
            draw_str = usr_input.substring(last_drawn_idx);
        }
        else if (state == WifiState::Password)
        {
            for (uint16_t i = last_drawn_idx; i < usr_input.length(); ++i)
            {
                draw_str += '*';
            }
        }
        last_drawn_idx = usr_input.length();
        ViewInterface::DrawInputString(draw_str);
    }
}

void WifiView::HandleInput()
{
    // Parse commands
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);

        if (usr_input == "/refresh" || usr_input == "/r")
        {
            manager.ClearSSIDs();
            last_num_ssids = -1;
            SendGetSSIDPacket();
            String msg = "UI: Refresh ssids\n\r";
            HAL_UART_Transmit(&huart1, (const uint8_t*)msg.c_str(), msg.length(), HAL_MAX_DELAY);
        }

        return;
    }

    HandleWifiInput();
}

void WifiView::HandleWifiInput()
{
    const uint16_t x_start = 1;
    const uint16_t y_start = 50;
    if (state == WifiState::SSID)
    {
        int32_t ssid_id = usr_input.ToNumber();
        if (ssid_id == -1)
        {
            request_msg = "Error. Please select SSID number:";
            redraw_menu = true;
            return;
        }

        if (ssids.find(ssid_id) == ssids.end())
        {
            request_msg = "Error. Please select SSID number:";
            redraw_menu = true;
            return;
        }

        ssid = ssids.at(ssid_id);
        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID,
            ssid.data(), ssid.length());

        // Clear user input
        screen.FillRectangle(x_start, y_start, screen.ViewWidth(), screen.ViewHeight(), C_BLACK);

        request_msg = "Please enter the wifi password";
        state = WifiState::Password;
        redraw_menu = true;
    }
    else if (state == WifiState::Password)
    {
        password = usr_input;
        state = WifiState::Connecting;

        setting_manager.SaveSetting(
            SettingManager::SettingAddress::SSID_Password,
            password.data(), password.length());

        // Clear user input
        screen.FillRectangle(x_start, y_start, screen.ViewWidth(), screen.ViewHeight(), C_BLACK);

        request_msg = "Connecting";
        redraw_menu = true;

        manager.ConnectToWifi();
    }
}

void WifiView::SendGetSSIDPacket()
{
    std::unique_ptr<Packet> ssid_req_packet = std::make_unique<Packet>();
    ssid_req_packet->SetData(Packet::Types::Command, 0, 6);
    ssid_req_packet->SetData(manager.NextPacketId(), 6, 8);
    ssid_req_packet->SetData(1, 14, 10);
    ssid_req_packet->SetData(Packet::Commands::SSIDs, 24, 8);
    manager.EnqueuePacket(std::move(ssid_req_packet));
}

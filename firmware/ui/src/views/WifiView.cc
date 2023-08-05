#include "WifiView.hh"

#include "WifiView.hh"
#include "UserInterfaceManager.hh"
#include "ChatView.hh"

WifiView::WifiView(UserInterfaceManager& manager,
                   Screen& screen,
                   Q10Keyboard& keyboard,
                   SettingManager& setting_manager)
    : ViewInterface(manager, screen, keyboard, setting_manager),
      last_num_ssids(0),
      next_get_ssid_timeout(0)
{
}

WifiView::~WifiView()
{

}

void WifiView::Update()
{
    // Periodically get the list of teams.
    SendGetSSIDPacket();

    return;
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
    }

    auto ssids = manager.SSIDs();
    if (last_num_ssids != ssids.size())
    {
        uint16_t idx = 0;
        for (auto ssid : ssids)
        {
            const uint16_t y_start = 50;

            // convert the ssid int val to a string
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

void WifiView::HandleInput()
{
    // Parse commands
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);
    }
    else
    {

    }
}

void WifiView::SendGetSSIDPacket()
{
    if (HAL_GetTick() < next_get_ssid_timeout)
        return;

    Packet* ssid_req_packet = new Packet();
    ssid_req_packet->SetData(Packet::Types::Command, 0, 6);
    ssid_req_packet->SetData(manager.NextPacketId(), 6, 8);
    ssid_req_packet->SetData(1, 14, 10);
    ssid_req_packet->SetData(Packet::Commands::SSIDs, 24, 8);
    manager.EnqueuePacket(ssid_req_packet);

    // Get the list again after 30 seconds
    next_get_ssid_timeout = HAL_GetTick() + 30000;
}

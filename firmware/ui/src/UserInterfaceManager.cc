#include "UserInterfaceManager.hh"
#include "String.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

#include "SettingManager.hh"

// Init the static var
uint8_t UserInterfaceManager::Packet_Id = 1;

UserInterfaceManager::UserInterfaceManager(Screen &screen,
                                           Q10Keyboard &keyboard,
                                           SerialInterface &net_interface,
                                           EEPROM& eeprom) :
    screen(&screen),
    keyboard(&keyboard),
    net_layer(&net_interface),
    setting_manager(eeprom),
    view(nullptr),
    received_messages(), // TODO limit?
    force_redraw(false),
    current_time(HAL_GetTick()),
    ssids(),
    last_wifi_check(0),
    is_connected_to_wifi(false)
{
    if (setting_manager.LoadSetting(SettingManager::SettingAddress::Firstboot)
        == FIRST_BOOT_DONE)
        ChangeView<LoginView>();
    else
        ChangeView<FirstBootView>();
}

UserInterfaceManager::~UserInterfaceManager()
{
    screen = nullptr;
    keyboard = nullptr;
    net_layer = nullptr;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
    // if (current_time > last_test_packet)
    // {
    //     SendTestPacket();
    //     last_test_packet = current_time + 5000;
    // }


    current_time = HAL_GetTick();

    view->Run();

    if (RedrawForced())
    {
        force_redraw = false;
        return;
    }

    if (current_time > last_wifi_check)
    {
        // Check a check wifi status packet
        Packet check_wifi;
        check_wifi.SetData(Packet::Types::Command, 0, 6);
        check_wifi.SetData(NextPacketId(), 6, 8);
        check_wifi.SetData(1, 14, 10);
        check_wifi.SetData(Packet::Commands::WifiStatus, 24, 8);

        EnqueuePacket(std::move(check_wifi));
        last_wifi_check = current_time + 5000;
    }

    // Run the receive and transmit
    net_layer.RxTx(current_time);

    // TODO we probably should keep a small list of the most recent messages
    //      in the user interface manager instead of chat view
    //      otherwise it will be bizarre having to get all of the old messages.
    // TODO this should only occur in the chat view mode?

    HandleIncomingPackets();
}

bool UserInterfaceManager::HasMessages()
{
    return received_messages.size();
}

Vector<Message>& UserInterfaceManager::GetMessages()
{
    return received_messages;
}

void UserInterfaceManager::ClearMessages()
{
    received_messages.clear();
}

void UserInterfaceManager::EnqueuePacket(Packet&& packet)
{
    // TODO maybe make this into a linked list?
    net_layer.EnqueuePacket(std::move(packet));
}

void UserInterfaceManager::ForceRedraw()
{
    force_redraw = true;
    // TODO change this to draw
    Run();
}

bool UserInterfaceManager::RedrawForced()
{
    return force_redraw;
}

uint32_t UserInterfaceManager::NextPacketId()
{
    if (Packet_Id == 0xFE)
        Packet_Id = 1;
    return UserInterfaceManager::Packet_Id++;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    if (!net_layer.HasRxPackets()) return;

    // Get the packets
    Vector<Packet*>& packets = net_layer.GetRxPackets();

    // Handle incoming packets
    while (packets.size() > 0)
    {
        // Get the type
        Packet& rx_packet = *packets[0];
        uint8_t p_type = rx_packet.GetData(0, 6);
        switch (p_type)
        {
            // P_type will only be message or debug by this point
            case (Packet::Types::Message):
            {
                // Write a message to the screen
                Message in_msg;
                // TODO The message should be parsed some how here.
                in_msg.Timestamp("00:00");
                in_msg.Sender("Server");

                String body;

                // Skip the type and length, add the whole message
                uint16_t packet_len = rx_packet.GetData(14, 10);
                for (uint32_t j = 0; j < packet_len; ++j)
                {
                    body.push_back((char)rx_packet.GetData(24 + (j * 8), 8));
                }

                in_msg.Body(body);
                received_messages.push_back(in_msg);
                break;
            }
            case (Packet::Types::Setting):
            {
                // TODO Set the setting

                // For settings we expect the data to dedicate 16 bits to the id
                // uint16_t packet_len = rx_packet.GetData(14, 10);

                // Get the setting id
                // uint16_t setting_id = rx_packet.GetData(24, 16);

                // Get the data from the packet and set the setting
                // for now we expect a 32 bit value for each setting
                // uint32_t setting_data = rx_packet.GetData(40, 32);

                break;
            }
            case (Packet::Types::Command):
            {
                // TODO move to a parse command function
                // TODO switch statement
                uint8_t command_type = rx_packet.GetData(24, 8);
                if (Packet::Commands::SSIDs == command_type)
                {
                    // Get the packet len
                    uint16_t len = rx_packet.GetData(14, 10);

                    // Get the ssid id
                    uint8_t ssid_id = rx_packet.GetData(32, 8);

                    // Build the string
                    String str;
                    for (uint8_t i = 0; i < len - 2; ++i)
                    {
                        str.push_back(static_cast<char>(
                            rx_packet.GetData(40 + i * 8, 8)));
                    }

                    ssids[ssid_id] = std::move(str);
                }
                else if (Packet::Commands::WifiStatus == command_type)
                {
                    is_connected_to_wifi = rx_packet.GetData(32, 8);
                }

                break;
            }
            default:
            {
                // We'll do nothing if it doesn't fit these types
            }
        }

        // TODO this should be automatic when the vector erases it?
        delete packets[0];
        packets.erase(0);
    }
}

const uint32_t UserInterfaceManager::GetTxStatusColour() const
{
    return GetStatusColour(net_layer.GetTxStatus());
}

const uint32_t UserInterfaceManager::GetRxStatusColour() const
{
    return GetStatusColour(net_layer.GetRxStatus());
}

const std::map<uint8_t, String>& UserInterfaceManager::SSIDs() const
{
    return ssids;
}

void UserInterfaceManager::ConnectToWifi()
{
    //TODO error checking
    // Load the ssid and password from eeprom
    char* ssid;
    unsigned char ssid_len = 0;
    setting_manager.LoadSetting(SettingManager::SettingAddress::SSID,
        &ssid, ssid_len);

    char* ssid_password;
    unsigned char ssid_password_len;
    setting_manager.LoadSetting(SettingManager::SettingAddress::SSID_Password,
        &ssid_password, ssid_password_len);

    // Create the packet
    Packet connect_packet;
    connect_packet.SetData(Packet::Types::Command, 0, 6);
    connect_packet.SetData(UserInterfaceManager::NextPacketId(), 6, 8);
    // THINK should these be separate packets?
    // +3 for the length of the ssid, length of the password
    uint16_t length = ssid_len + ssid_password_len + 3;
    connect_packet.SetData(length, 14, 10);

    connect_packet.SetData(Packet::Commands::ConnectToSSID, 24, 8);

    // Set the length of the ssid
    connect_packet.SetData(ssid_len, 32, 8);

    // Populate with the ssid
    uint16_t i;
    uint16_t offset = 40;
    for (i = 0; i < ssid_len; ++i)
    {
        connect_packet.SetData(ssid[i], offset, 8);
        offset += 8;
    }

    // Set the length of the password
    connect_packet.SetData(ssid_password_len, offset, 8);
    offset += 8;

    // Populate with the password
    uint16_t j;
    for (j = 0; j < ssid_password_len; ++j)
    {
        connect_packet.SetData(ssid_password[j], offset, 8);
        offset += 8;
    }

    // Enqueue the message
    EnqueuePacket(std::move(connect_packet));

    delete ssid;
    delete ssid_password;
}

const bool UserInterfaceManager::IsConnectedToWifi() const
{
    return is_connected_to_wifi;
}

const uint32_t UserInterfaceManager::GetStatusColour(
    const SerialManager::SerialStatus status) const
{
    if (status == SerialManager::SerialStatus::OK)
        return C_GREEN;
    else if (status == SerialManager::SerialStatus::PARTIAL)
        return C_CYAN;
    else if (status == SerialManager::SerialStatus::TIMEOUT)
        return C_MAGENTA;
    else if (status == SerialManager::SerialStatus::BUSY)
        return C_YELLOW;
    else if (status == SerialManager::SerialStatus::ERROR)
        return C_RED;
    else
        return C_WHITE;
}

const void UserInterfaceManager::SendTestPacket()
{
    Packet test_packet(current_time);

    test_packet.SetData(Packet::Types::Message, 0, 6);
    test_packet.SetData(NextPacketId(), 6, 8);
    test_packet.SetData(5, 14, 10);
    test_packet.SetData('H', 24, 8);
    test_packet.SetData('e', 32, 8);
    test_packet.SetData('l', 40, 8);
    test_packet.SetData('l', 48, 8);
    test_packet.SetData('o', 56, 8);

    EnqueuePacket(std::move(test_packet));
}

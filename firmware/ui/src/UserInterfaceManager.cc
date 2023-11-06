#include "UserInterfaceManager.hh"
#include "String.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

#include "SettingManager.hh"
#include "QChat.hh"

#include "main.hh"

// Init the static var

UserInterfaceManager::UserInterfaceManager(Screen& screen,
    Q10Keyboard& keyboard,
    SerialInterface& net_interface,
    EEPROM& eeprom):
    screen(&screen),
    keyboard(&keyboard),
    net_layer(&net_interface),
    setting_manager(eeprom),
    view(nullptr),
    received_messages(), // TODO limit?
    force_redraw(false),
    current_time(HAL_GetTick()),
    ssids(),
    last_wifi_check(10000),
    is_connected_to_wifi(false),
    attempt_to_connect_timeout(0),
    username("")
{
    if (setting_manager.LoadSetting(SettingManager::SettingAddress::Firstboot)
        == FIRST_BOOT_DONE)
    {
        LoadSettings();
        ChangeView<LoginView>();
    }
    else
    {
        ChangeView<FirstBootView>();
    }
}

UserInterfaceManager::~UserInterfaceManager()
{
    screen = nullptr;
    keyboard = nullptr;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
    current_time = HAL_GetTick();

    view->Run();

    if (RedrawForced())
    {
        force_redraw = false;
        return;
    }

    SendCheckWifiPacket();

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

void UserInterfaceManager::EnqueuePacket(Packet* packet)
{
    // TODO maybe make this into a linked list?
    net_layer.EnqueuePacket(packet);
}

void UserInterfaceManager::LoopbackPacket(Packet* packet)
{
    net_layer.LoopbackRxPacket(packet);
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

uint8_t UserInterfaceManager::NextPacketId()
{
    return net_layer.NextPacketId();
}

const String& UserInterfaceManager::GetUsername()
{
    if (username == "")
    {
        LoadUsername();
    }

    return username;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    if (!net_layer.HasRxPackets()) return;

    // Get the packets
    const Vector<Packet*>& packets = net_layer.GetRxPackets();

    // Handle incoming packets
    while (packets.size() > 0)
    {
        // Get the type
        Packet* rx_packet = packets[0];
        uint8_t p_type = rx_packet->GetData(0, 6);
        switch (p_type)
        {
            // P_type will only be message or debug by this point
            case (Packet::Types::Message):
            {
                HandleMessagePacket(rx_packet);

                // HACK remove later
                qchat::Ascii* ascii = ascii_messages[0];

                Message in_msg;
                in_msg.Timestamp("00.00");
                in_msg.Sender("Ascii");

                in_msg.Body(ascii->message.c_str());

                received_messages.push_back(in_msg);

                delete ascii;
                ascii_messages.erase(0);

                // Write a message to the screen
                // Message in_msg;
                // // TODO The message should be parsed some how here.
                // in_msg.Timestamp("00:00");
                // in_msg.Sender("Server");

                // String body;

                // // Skip the type and length, add the whole message
                // uint16_t packet_len = rx_packet->GetData(14, 10);
                // for (uint32_t j = 0; j < packet_len; ++j)
                // {
                //     body.push_back((char)rx_packet->GetData(24 + (j * 8), 8));
                // }

                // in_msg.Body(body);
                // received_messages.push_back(in_msg);
                break;
            }
            case (Packet::Types::Setting):
            {
                // TODO Set the setting

                // For settings we expect the data to dedicate 16 bits to the id
                // uint16_t packet_len = rx_packet->GetData(14, 10);

                // Get the setting id
                // uint16_t setting_id = rx_packet->GetData(24, 16);

                // Get the data from the packet and set the setting
                // for now we expect a 32 bit value for each setting
                // uint32_t setting_data = rx_packet->GetData(40, 32);

                break;
            }
            case (Packet::Types::Command):
            {
                // TODO move to a parse command function
                // TODO switch statement
                uint8_t command_type = rx_packet->GetData(24, 8);
                if (Packet::Commands::SSIDs == command_type)
                {
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
                else if (Packet::Commands::WifiStatus == command_type)
                {
                    // Response from the esp32 will invoke this
                    is_connected_to_wifi = rx_packet->GetData(32, 8);
                    if (!is_connected_to_wifi && HAL_GetTick() > attempt_to_connect_timeout)
                    {
                        ConnectToWifi();

                        // Wait a long time before trying to connect again
                        attempt_to_connect_timeout = HAL_GetTick() + 10000;
                    }
                }

                break;
            }
            default:
            {
                // We'll do nothing if it doesn't fit these types
                break;
            }
        }

        // delete rx_packet;
        // packets.erase(0);

        net_layer.DestroyRxPacket(0);
    }
}

uint32_t UserInterfaceManager::GetTxStatusColour() const
{
    return GetStatusColour(net_layer.GetTxStatus());
}

uint32_t UserInterfaceManager::GetRxStatusColour() const
{
    return GetStatusColour(net_layer.GetRxStatus());
}

const std::map<uint8_t, String>& UserInterfaceManager::SSIDs() const
{
    return ssids;
}

void UserInterfaceManager::ClearSSIDs()
{
    ssids.clear();
}

void UserInterfaceManager::ConnectToWifi()
{
    //TODO error checking
    // Load the ssid and password from eeprom
    int8_t* ssid;
    int16_t ssid_len = 0;
    if (!setting_manager.LoadSetting(SettingManager::SettingAddress::SSID,
        &ssid, ssid_len)) return;

    int8_t* ssid_password;
    int16_t ssid_password_len = 0;
    if (!setting_manager.LoadSetting(
        SettingManager::SettingAddress::SSID_Password, &ssid_password,
        ssid_password_len)) return;

    String ssid_str;
    for (int i = 0 ; i < ssid_len; i++)
        ssid_str += ssid[i];
    String password_str;
    for (int i = 0 ; i < ssid_password_len; i++)
        password_str += ssid_password[i];

    // Create the packet
    Packet* connect_packet = new Packet();
    connect_packet->SetData(Packet::Types::Command, 0, 6);
    connect_packet->SetData(UserInterfaceManager::NextPacketId(), 6, 8);
    // THINK should these be separate packets?
    // +3 for the length of the ssid, length of the password
    uint16_t length = ssid_len + ssid_password_len + 3;
    connect_packet->SetData(length, 14, 10);

    connect_packet->SetData(Packet::Commands::ConnectToSSID, 24, 8);

    // Set the length of the ssid
    connect_packet->SetData(ssid_len, 32, 8);

    // Populate with the ssid
    uint16_t i;
    uint16_t offset = 40;
    for (i = 0; i < ssid_len; ++i)
    {
        connect_packet->SetData(ssid[i], offset, 8);
        offset += 8;
    }

    // Set the length of the password
    connect_packet->SetData(ssid_password_len, offset, 8);
    offset += 8;

    // Populate with the password
    uint16_t j;
    for (j = 0; j < ssid_password_len; ++j)
    {
        connect_packet->SetData(ssid_password[j], offset, 8);
        offset += 8;
    }

    // Enqueue the message
    EnqueuePacket(connect_packet);

    delete ssid;
    delete ssid_password;
}

void UserInterfaceManager::ConnectToWifi(const String& ssid,
    const String& password)
{
    Packet* connect_packet = new Packet();
    connect_packet->SetData(Packet::Types::Command, 0, 6);
    connect_packet->SetData(UserInterfaceManager::NextPacketId(), 6, 8);

    uint16_t ssid_len = ssid.length();
    uint16_t ssid_password_len = password.length();
    uint16_t length = ssid_len + ssid_password_len + 3;
    connect_packet->SetData(length, 14, 10);

    connect_packet->SetData(Packet::Commands::ConnectToSSID, 24, 8);

    // Set the length of the ssid
    connect_packet->SetData(ssid_len, 32, 8);

    // Populate with the ssid
    uint16_t i;
    uint16_t offset = 40;
    for (i = 0; i < ssid_len; ++i)
    {
        connect_packet->SetData(ssid[i], offset, 8);
        offset += 8;
    }

    // Set the length of the password
    connect_packet->SetData(ssid_password_len, offset, 8);
    offset += 8;

    // Populate with the password
    uint16_t j;
    for (j = 0; j < ssid_password_len; ++j)
    {
        connect_packet->SetData(password[j], offset, 8);
        offset += 8;
    }

    // Enqueue the message
    EnqueuePacket(connect_packet);
}

bool UserInterfaceManager::IsConnectedToWifi() const
{
    return is_connected_to_wifi;
}

uint32_t UserInterfaceManager::GetStatusColour(
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
    else if (status == SerialManager::SerialStatus::CRITICAL_ERROR)
        return C_BLUE;
    else
        return C_WHITE;
}

void UserInterfaceManager::SendTestPacket()
{
    Packet* test_packet = new Packet();

    test_packet->SetData(Packet::Types::Message, 0, 6);
    test_packet->SetData(NextPacketId(), 6, 8);
    test_packet->SetData(5, 14, 10);
    test_packet->SetData('H', 24, 8);
    test_packet->SetData('e', 32, 8);
    test_packet->SetData('l', 40, 8);
    test_packet->SetData('l', 48, 8);
    test_packet->SetData('o', 56, 8);

    EnqueuePacket(test_packet);
}

void UserInterfaceManager::SendCheckWifiPacket()
{
    if (current_time < last_wifi_check) return;

    // Check a check wifi status packet
    Packet* check_wifi = new Packet();

    // Set the command
    check_wifi->SetData(Packet::Types::Command, 0, 6);

    // Set the id
    check_wifi->SetData(NextPacketId(), 6, 8);

    // Set the size
    check_wifi->SetData(1, 14, 10);

    // Set the data
    check_wifi->SetData(Packet::Commands::WifiStatus, 24, 8);

    EnqueuePacket(check_wifi);

    last_wifi_check = current_time + 10000;
    uint8_t message [] = "UI: Send check wifi to esp\n\r";
    HAL_UART_Transmit(&huart1, message, sizeof(message) / sizeof(char), 1000);
}

void UserInterfaceManager::HandleMessagePacket(
    Packet* packet)
{
    // Get the message type
    qchat::MessageTypes message_type =
        (qchat::MessageTypes)packet->GetData(24, 8);

    // Check the message type
    if (message_type == qchat::MessageTypes::Ascii)
    {
        // Make a new the ascii message pointer
        qchat::Ascii* ascii = new qchat::Ascii();

        // Decode the packet
        const bool res = qchat::Codec::decode(*ascii, packet, 32);

        if (!res)
        {
            // TODO some error state
            return;
        }

        // Do something with the ascii message
        ascii_messages.push_back(ascii);
    }
}

void UserInterfaceManager::LoadSettings()
{
    LoadUsername();
}

void UserInterfaceManager::LoadUsername()
{
    // TODO make this better
    char* username;
    short len;
    setting_manager.LoadSetting(SettingManager::SettingAddress::Username,
        &username, len);
    for (short i = 0; i < len; i++)
    {
        this->username.push_back(username[i]);
    }
    delete username;
    // ^^^ ugly chunk
}
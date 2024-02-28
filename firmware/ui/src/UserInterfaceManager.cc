#include "UserInterfaceManager.hh"
#include "String.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

#include "SettingManager.hh"
#include "QChat.hh"

#include "main.h"
extern UART_HandleTypeDef huart1;

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
    last_wifi_check(10000),
    is_connected_to_wifi(false),
    attempt_to_connect_timeout(0),
    active_room(nullptr)
{
    if (setting_manager.ReadSetting(SettingManager::SettingAddress::Firstboot)
        == FIRST_BOOT_DONE)
    {
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

    // TODO Clear pending packets after a certain amount of time
}

bool UserInterfaceManager::HasNewMessages()
{
    return has_new_messages;
}

std::vector<String> UserInterfaceManager::TakeMessages()
{
    has_new_messages = false;
    auto out = std::vector<String>{};
    std::swap(received_messages, out);
    return out;
}

void UserInterfaceManager::PushMessage(String&& str)
{
    has_new_messages = true;
    received_messages.push_back(str);
}

void UserInterfaceManager::ClearMessages()
{
    received_messages.clear();
}

void UserInterfaceManager::EnqueuePacket(std::unique_ptr<SerialPacket> packet)
{
    // TODO maybe make this into a linked list?
    net_layer.EnqueuePacket(std::move(packet));
}

void UserInterfaceManager::LoopbackPacket(std::unique_ptr<SerialPacket> packet)
{
    net_layer.LoopbackRxPacket(std::move(packet));
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

uint16_t UserInterfaceManager::NextPacketId()
{
    return net_layer.NextPacketId();
}

void UserInterfaceManager::ChangeRoom(std::unique_ptr<qchat::Room> new_room)
{
    if (active_room != nullptr)
    {
        // If we are currently in a room we need to unsubscribe and delete
        // any messages that are currently in the map.
        qchat::UnwatchRoom unwatch;
        unwatch.room_uri = active_room->room_uri;

        std::unique_ptr<SerialPacket> unwatch_packet = std::make_unique<SerialPacket>();
        unwatch_packet->SetData(SerialPacket::Types::Message, 0, 1);
        unwatch_packet->SetData(NextPacketId(), 1, 2);

        qchat::Codec::encode(unwatch_packet, unwatch);

        // Push the packet onto the queue
        EnqueuePacket(std::move(unwatch_packet));

        active_room.reset();
    }

    // Send a watch message to the new room
    active_room = std::move(new_room);

    // TODO placeholder for better more suitable code
    // TODO Active room needs to be passed to the chat view some how
    std::string user_name{ setting_manager.Username()->c_str() };

    // Set watch on the room
    qchat::WatchRoom watch(active_room->publisher_uri + user_name + "/", active_room->room_uri);

    std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>(HAL_GetTick(), 5);
    packet->SetData(SerialPacket::Types::Message, 0, 1);
    packet->SetData(NextPacketId(), 1, 2);

    qchat::Codec::encode(packet, watch);
    uint32_t new_offset = packet->NumBytes();

    // Expiry time
    packet->SetData(0xFFFFFFFF, new_offset, 4);
    new_offset += 4;

    // Creation time
    packet->SetData(0, new_offset, 4);
    new_offset += 4;
    EnqueuePacket(std::move(packet));
}

const std::unique_ptr<qchat::Room>& UserInterfaceManager::ActiveRoom() const
{
    return active_room;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    if (!net_layer.HasRxPackets()) return;

    // Get the packets
    const Vector<std::unique_ptr<SerialPacket>>& packets = net_layer.GetRxPackets();

    // Handle incoming packets
    while (packets.size() > 0)
    {
        // Get the type
        std::unique_ptr<SerialPacket> rx_packet = std::move(packets[0]);
        net_layer.DestroyRxPacket(0);

        uint8_t p_type = rx_packet->GetData<uint8_t>(0, 1);
        switch (p_type)
        {
            // P_type will only be message or debug by this point
            case (SerialPacket::Types::Message):
            {
                HandleMessagePacket(std::move(rx_packet));

                if (ascii_messages.size() > 0)
                {
                    // HACK remove later
                    qchat::Ascii* ascii = ascii_messages[0];

                    received_messages.push_back(ascii->message.c_str());
                    has_new_messages = true;

                    // HACK remove later
                    delete ascii;
                    ascii_messages.erase(0);
                }

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
            case (SerialPacket::Types::Setting):
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
            case (SerialPacket::Types::Command):
            {
                // TODO move into a function too many tabs deeeeep
                SerialPacket::Commands command_type = static_cast<SerialPacket::Commands>(
                    rx_packet->GetData<uint8_t>(5, 1));

                switch (command_type)
                {
                    case SerialPacket::Commands::WifiStatus:
                    {
                        // TODO move into a wifi handler
                        // Response from the esp32 will invoke this
                        uint8_t message [] = "Got a connection status\n\r";
                        HAL_UART_Transmit(&huart1, message, sizeof(message) / sizeof(char), 1000);
                        is_connected_to_wifi = rx_packet->GetData<char>(6, 1);
                        if (!is_connected_to_wifi && HAL_GetTick() > attempt_to_connect_timeout)
                        {
                            ConnectToWifi();

                            // Wait a long time before trying to connect again
                            attempt_to_connect_timeout = HAL_GetTick() + 10000;
                        }
                        break;
                    }
                    default:
                    {
                        // Every other command type should be put into the
                        // pending command packets.
                        pending_command_packets[command_type].Write(std::move(rx_packet));
                        break;
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

        // net_layer.DestroyRxPacket(0);
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

bool UserInterfaceManager::GetReadyPackets(
    RingBuffer<std::unique_ptr<SerialPacket>>** buff,
    const SerialPacket::Commands command_type) const
{
    if (pending_command_packets.find(command_type) ==
        pending_command_packets.end())
    {
        return false;
    }

    *buff = const_cast<RingBuffer<std::unique_ptr<SerialPacket>>*>(
        &pending_command_packets.at(command_type));

    return true;
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

    delete [] ssid;
    delete [] ssid_password;

    ConnectToWifi(ssid_str, password_str);

    // Create the packet
    // std::unique_ptr<SerialPacket> connect_packet = std::make_unique<SerialPacket>();
    // connect_packet->SetData(SerialPacket::Types::Command, 0, 1);
    // connect_packet->SetData(UserInterfaceManager::NextPacketId(), 1, 2);
    // // THINK should these be separate packets?
    // // +3 for the length of the ssid, length of the password
    // uint16_t length = ssid_len + ssid_password_len + 3;
    // connect_packet->SetData(length, 3, 2);

    // connect_packet->SetData(SerialPacket::Commands::WifiConnect, 5, 1);

    // // Set the length of the ssid
    // connect_packet->SetData(ssid_len, 6, 2);

    // // Populate with the ssid
    // uint16_t i;
    // uint16_t offset = 8;
    // for (i = 0; i < ssid_len; ++i)
    // {
    //     connect_packet->SetData(ssid[i], offset, 1);
    //     offset += 1;
    // }

    // // Set the length of the password
    // connect_packet->SetData(ssid_password_len, offset, 2);
    // offset += 2;

    // // Populate with the password
    // uint16_t j;
    // for (j = 0; j < ssid_password_len; ++j)
    // {
    //     connect_packet->SetData(ssid_password[j], offset, 1);
    //     offset += 1;
    // }

    // // Enqueue the message
    // EnqueuePacket(std::move(connect_packet));
}

void UserInterfaceManager::ConnectToWifi(const String& ssid,
    const String& password)
{
    std::unique_ptr<SerialPacket> connect_packet = std::make_unique<SerialPacket>(HAL_GetTick());
    connect_packet->SetData(SerialPacket::Types::Command, 0, 1);
    connect_packet->SetData(UserInterfaceManager::NextPacketId(), 1, 2);

    uint16_t ssid_len = ssid.length();
    uint16_t ssid_password_len = password.length();
    uint16_t length = ssid_len + ssid_password_len + 5;
    connect_packet->SetData(length, 3, 2);

    connect_packet->SetData(SerialPacket::Commands::WifiConnect, 5, 1);

    // Set the length of the ssid
    connect_packet->SetData(ssid_len, 6, 2);

    // Populate with the ssid
    uint16_t i;
    uint16_t offset = 8;
    for (i = 0; i < ssid_len; ++i)
    {
        connect_packet->SetData(ssid[i], offset, 1);
        offset += 1;
    }

    // Set the length of the password
    connect_packet->SetData(ssid_password_len, offset, 2);
    offset += 2;

    // Populate with the password
    uint16_t j;
    for (j = 0; j < ssid_password_len; ++j)
    {
        connect_packet->SetData(password[j], offset, 1);
        offset += 1;
    }

    // Enqueue the message
    EnqueuePacket(std::move(connect_packet));
}

bool UserInterfaceManager::IsConnectedToWifi() const
{
    return is_connected_to_wifi;
}

uint32_t UserInterfaceManager::GetStatusColour(
    const SerialPacketManager::SerialStatus status) const
{
    if (status == SerialPacketManager::SerialStatus::OK)
        return C_GREEN;
    else if (status == SerialPacketManager::SerialStatus::PARTIAL)
        return C_CYAN;
    else if (status == SerialPacketManager::SerialStatus::TIMEOUT)
        return C_MAGENTA;
    else if (status == SerialPacketManager::SerialStatus::BUSY)
        return C_YELLOW;
    else if (status == SerialPacketManager::SerialStatus::ERROR)
        return C_RED;
    else if (status == SerialPacketManager::SerialStatus::CRITICAL_ERROR)
        return C_BLUE;
    else
        return C_WHITE;
}

void UserInterfaceManager::SendTestPacket()
{
    std::unique_ptr<SerialPacket> test_packet = std::make_unique<SerialPacket>();

    test_packet->SetData(SerialPacket::Types::Message, 0, 1);
    test_packet->SetData(NextPacketId(), 1, 2);
    test_packet->SetData(5, 3, 2);
    test_packet->SetData('H', 5, 1);
    test_packet->SetData('e', 6, 1);
    test_packet->SetData('l', 7, 1);
    test_packet->SetData('l', 8, 1);
    test_packet->SetData('o', 9, 1);

    EnqueuePacket(std::move(test_packet));
}

void UserInterfaceManager::SendCheckWifiPacket()
{
    if (current_time < last_wifi_check) return;

    // Check a check wifi status packet
    std::unique_ptr<SerialPacket> check_wifi = std::make_unique<SerialPacket>(HAL_GetTick());

    // Set the command
    check_wifi->SetData(SerialPacket::Types::Command, 0, 1);

    // Set the id
    check_wifi->SetData(NextPacketId(), 1, 2);

    // Set the size
    check_wifi->SetData(1, 3, 2);

    // Set the data
    check_wifi->SetData(SerialPacket::Commands::WifiStatus, 5, 1);

    uint8_t message [] = "UI: Send check wifi to esp\n\r";
    HAL_UART_Transmit(&huart1, message, sizeof(message) / sizeof(char), HAL_MAX_DELAY);

    unsigned char* buff = check_wifi->Data();
    for (uint16_t i = 0; i < check_wifi->NumBytes(); ++i)
    {
        HAL_UART_Transmit(&huart1, buff + i, 1, 1000);
    }

    EnqueuePacket(std::move(check_wifi));

    last_wifi_check = current_time + 10000;
}

void UserInterfaceManager::HandleMessagePacket(
    std::unique_ptr<SerialPacket> packet)
{
    // Get the message type
    qchat::MessageTypes message_type =
        (qchat::MessageTypes)packet->GetData<uint8_t>(5, 1);

    // Check the message type
    if (message_type == qchat::MessageTypes::Ascii)
    {
        // Make a new the ascii message pointer
        qchat::Ascii* ascii = new qchat::Ascii();

        // message uri
        uint32_t uri_len = packet->GetData<uint32_t>(6, 4);
        uint32_t offset = 10;
        for (uint16_t i = 0; i < uri_len; ++i)
        {
            ascii->message_uri.push_back(
                packet->GetData<char>(offset, 1));
            offset += 1;
        }

        // ascii
        uint32_t msg_len = packet->GetData<uint32_t>(offset, 4);
        offset += 4;

        for (uint16_t i = 0; i < msg_len; ++i)
        {
            ascii->message.push_back(static_cast<char>(
                packet->GetData<uint32_t>(offset, 1)));
            offset += 1;
        }

        // Decode the packet
        // const bool res = qchat::Codec::decode(*ascii, packet, 32);

        // if (!res)
        // {
        //     // TODO some error state
        //     return;
        // }

        // Do something with the ascii message
        ascii_messages.push_back(ascii);
    }
}

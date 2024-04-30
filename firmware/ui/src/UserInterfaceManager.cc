#include "UserInterfaceManager.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "TeamView.hh"
#include "ChatView.hh"

#include "SettingManager.hh"
#include "QChat.hh"

#include "main.h"

#include <string>
#include <vector>

extern UART_HandleTypeDef huart1;

// Init the static var

UserInterfaceManager::UserInterfaceManager(Screen &screen,
                                           Q10Keyboard &keyboard,
                                           SerialInterface &net_interface,
                                           EEPROM &eeprom) : screen(&screen),
                                                             keyboard(&keyboard),
                                                             net_layer(&net_interface),
                                                             setting_manager(eeprom),
                                                             view(nullptr),
                                                             network(setting_manager, net_layer, screen),
                                                             received_messages(), // TODO limit?
                                                             force_redraw(false),
                                                             current_tick(HAL_GetTick()),
                                                             last_wifi_check(10000),
                                                             is_connected_to_wifi(false),
                                                             attempt_to_connect_timeout(0),
                                                             active_room(nullptr)
{
    if (setting_manager.ReadSetting(SettingManager::SettingAddress::Firstboot) == FIRST_BOOT_DONE)
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
    current_tick = HAL_GetTick();

    // TODO rename to update
    view->Run();

    // TODO move into view
    if (RedrawForced())
    {
        force_redraw = false;
        return;
    }

    // TODO move into wifi update.
    // SendCheckWifiPacket();

    // Run the receive and transmit
    // TODO rename to Update
    net_layer.RxTx(current_tick);

    network.Update(current_tick);

    // TODO we probably should keep a small list of the most recent messages
    //      in the user interface manager instead of chat view
    //      otherwise it will be bizarre having to get all of the old messages.
    // TODO this should only occur in the chat view mode?
    HandleIncomingPackets(); // TODO remake this function

    // TODO Clear pending packets after a certain amount of time
}

bool UserInterfaceManager::HasNewMessages()
{
    return has_new_messages;
}

std::vector<std::string> UserInterfaceManager::TakeMessages()
{
    has_new_messages = false;
    auto out = std::vector<std::string>{};
    received_messages.swap(out);
    return out;
}

void UserInterfaceManager::PushMessage(std::string &&str)
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
        unwatch_packet->SetData(SerialPacket::Types::QMessage, 0, 1);
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
    std::string user_name = *setting_manager.Username();

    // Set watch on the room
    qchat::WatchRoom watch(active_room->publisher_uri + user_name + "/", active_room->room_uri);

    std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>(HAL_GetTick(), 5);
    packet->SetData(SerialPacket::Types::QMessage, 0, 1);
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

const std::unique_ptr<qchat::Room> &UserInterfaceManager::ActiveRoom() const
{
    return active_room;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    // TODO move this into the serialpacketmanager.
    if (!net_layer.HasRxPackets())
        return;

    auto& packets = net_layer.GetRxPackets();
    for (auto& rx_packet : packets)
    {
        SerialPacket::Types p_type = static_cast<SerialPacket::Types>(
            rx_packet->GetData<uint8_t>(0, 1));
        switch (p_type)
        {
        // P_type will only be message or debug by this point
        case (SerialPacket::Types::QMessage):
        {
            HandleMessagePacket(std::move(rx_packet));

            if (ascii_messages.size() > 0)
            {
                auto&& ascii = ascii_messages.front();
                PushMessage(std::move(ascii.message));
                ascii_messages.pop_front();
            }
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
        // THIS WILL NEVER BE CALLED NOW
        case (SerialPacket::Types::Command):
        {
            // // TODO move into a function too many tabs deeeeep
            // SerialPacket::Commands command_type = static_cast<SerialPacket::Commands>(
            //     rx_packet->GetData<uint8_t>(5, 2));

            // switch (command_type)
            // {
            // case SerialPacket::Commands::Wifi:
            // {
            //     // TODO move into a wifi handler
            //     // Response from the esp32 will invoke this
            //     Logger::Log(Logger::Level::Info, "Got a connection status");

            //     is_connected_to_wifi = rx_packet->GetData<char>(6, 1);
            //     if (!is_connected_to_wifi && HAL_GetTick() > attempt_to_connect_timeout)
            //     {
            //         ConnectToWifi();

            //         // Wait a long time before trying to connect again
            //         attempt_to_connect_timeout = HAL_GetTick() + 10000;
            //     }
            //     break;
            // }
            // default:
            // {
            //     // Every other command type should be put into the
            //     // pending command packets.
            //     pending_command_packets[command_type].Write(std::move(rx_packet));
            //     break;
            // }
            // }

            break;
        }
        default:
        {
            // We'll do nothing if it doesn't fit these types
            break;
        }
        }
    }
    packets.clear();
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
    RingBuffer<std::unique_ptr<SerialPacket>> **buff,
    const SerialPacket::Commands command_type) const
{
    if (pending_command_packets.find(command_type) ==
        pending_command_packets.end())
    {
        return false;
    }

    *buff = const_cast<RingBuffer<std::unique_ptr<SerialPacket>> *>(
        &pending_command_packets.at(command_type));

    return true;
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

    test_packet->SetData(SerialPacket::Types::QMessage, 0, 1);
    test_packet->SetData(NextPacketId(), 1, 2);
    test_packet->SetData(5, 3, 2);
    test_packet->SetData('H', 5, 1);
    test_packet->SetData('e', 6, 1);
    test_packet->SetData('l', 7, 1);
    test_packet->SetData('l', 8, 1);
    test_packet->SetData('o', 9, 1);

    EnqueuePacket(std::move(test_packet));
}

void UserInterfaceManager::HandleMessagePacket(
    std::unique_ptr<SerialPacket> packet)
{
    // Get the message type
    qchat::MessageTypes message_type =
        (qchat::MessageTypes)packet->GetData<uint8_t>(5, 1);

    // Check the message type
    switch (message_type)
    {
        case qchat::MessageTypes::Ascii:
        {
            // Make a new the ascii message pointer
            qchat::Ascii ascii;

            // message uri
            uint32_t uri_len = packet->GetData<uint32_t>(6, 4);
            uint32_t offset = 10;
            for (uint16_t i = 0; i < uri_len; ++i)
            {
                ascii.message_uri.push_back(
                    packet->GetData<char>(offset, 1));
                offset += 1;
            }

            // ascii
            uint32_t msg_len = packet->GetData<uint32_t>(offset, 4);
            offset += 4;

            for (uint16_t i = 0; i < msg_len; ++i)
            {
                ascii.message.push_back(static_cast<char>(
                    packet->GetData<uint32_t>(offset, 1)));
                offset += 1;
            }

            // Do something with the ascii message
            ascii_messages.push_back(std::move(ascii));
            break;
        }
        case (qchat::MessageTypes::WatchOk):
        {
            ChangeView<ChatView>();
            break;
        }
        default:
            break;
    }
}

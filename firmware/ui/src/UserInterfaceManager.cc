#include "UserInterfaceManager.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "TeamView.hh"
#include "ChatView.hh"

#include "SettingManager.hh"
#include "QChat.hh"

#include "audio_codec.hh"

#include "main.h"

#include <string>
#include <vector>

extern UART_HandleTypeDef huart1;

// Init the static var

UserInterfaceManager::UserInterfaceManager(Screen& screen,
    Q10Keyboard& keyboard,
    SerialInterface& net_interface,
    EEPROM& eeprom,
    AudioChip& audio
):
    screen(screen),
    keyboard(keyboard),
    net_layer(&net_interface),
    setting_manager(eeprom),
    audio(audio),
    view(nullptr),
    network(setting_manager, net_layer),
    received_messages(), // TODO limit?
    force_redraw(false),
    current_tick(HAL_GetTick()),
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
    delete view;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
    current_tick = HAL_GetTick();

    // TODO rename to update
    // TODO send update every 1/60 of a second?
    view->Run(current_tick);

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
    Logger::Log(Logger::Level::Debug, "take messages");
    has_new_messages = false;
    auto out = std::vector<std::string>{};
    received_messages.swap(out);
    return out;
}

void UserInterfaceManager::PushMessage(std::string&& str)
{
    has_new_messages = true;
    received_messages.push_back(str);
}

void UserInterfaceManager::ClearMessages()
{
    received_messages.clear();
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

// uint16_t UserInterfaceManager::NextPacketId()
// {
//     return net_layer.NextPacketId();
// }

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
        unwatch_packet->SetData(net_layer.NextPacketId(), 1, 2);

        qchat::Codec::encode(unwatch_packet, unwatch);

        // Push the packet onto the queue
        net_layer.EnqueuePacket(std::move(unwatch_packet));

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
    packet->SetData(net_layer.NextPacketId(), 1, 2);

    qchat::Codec::encode(packet, watch);
    uint32_t new_offset = packet->NumBytes();

    // Expiry time
    packet->SetData(0xFFFFFFFF, new_offset, 4);
    new_offset += 4;

    // Creation time
    packet->SetData(0, new_offset, 4);
    new_offset += 4;
    net_layer.EnqueuePacket(std::move(packet));
}

const std::unique_ptr<qchat::Room>& UserInterfaceManager::ActiveRoom() const
{
    return active_room;
}

void UserInterfaceManager::HandleIncomingPackets()
{
    // TODO move this into the serial packet manager.
    if (!net_layer.HasRxPackets())
    {
        return;
    }

    Logger::Log(Logger::Level::Info,"Handle incoming packets ", net_layer.GetRxPackets().size());

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
            default:
            {
                // We'll do nothing if it doesn't fit these types
                break;
            }
        }
    }
    packets.clear();
}

void UserInterfaceManager::SendTestPacket()
{
    std::unique_ptr<SerialPacket> test_packet = std::make_unique<SerialPacket>();

    test_packet->SetData(SerialPacket::Types::QMessage, 0, 1);
    test_packet->SetData(net_layer.NextPacketId(), 1, 2);
    test_packet->SetData(5, 3, 2);
    test_packet->SetData('H', 5, 1);
    test_packet->SetData('e', 6, 1);
    test_packet->SetData('l', 7, 1);
    test_packet->SetData('l', 8, 1);
    test_packet->SetData('o', 9, 1);

    net_layer.EnqueuePacket(std::move(test_packet));
}

void UserInterfaceManager::HandleMessagePacket(
    std::unique_ptr<SerialPacket> packet)
{
    // Get the message type
    qchat::MessageTypes message_type =
        (qchat::MessageTypes)packet->GetData<uint8_t>(5, 1);

    Logger::Log(Logger::Level::Info, "Qmessage type = ", (int)message_type);

    // Check the message type
    switch (message_type)
    {
        case qchat::MessageTypes::Ascii:
        {
            // This isn't being used.
            std::string message_uri;
            uint32_t uri_len = packet->GetData<uint32_t>(6, 4);
            uint32_t offset = 10;
            for (uint16_t i = 0; i < uri_len; ++i)
            {
                message_uri.push_back(
                    packet->GetData<char>(offset, 1));
                offset += 1;
            }

            // Get the length of the data
            // -1 for the type
            uint32_t data_len = packet->GetData<uint32_t>(offset, 4) - 1;
            offset += 4;

            // Get the real message type.
            message_type = (qchat::MessageTypes)packet->GetData<uint8_t>(offset, 1);
            offset++;
            Logger::Log(Logger::Level::Info, "True message type = ", (int)message_type);

            switch (message_type)
            {
                case (qchat::MessageTypes::Ascii):
                {
                    // Make a new the ascii message pointer
                    qchat::Ascii ascii;

                    // message uri
                    ascii.message_uri = message_uri;

                    Logger::Log(Logger::Level::Info, "Message uri output = ", ascii.message_uri);

                    for (uint16_t i = 0; i < data_len; ++i)
                    {
                        ascii.message.push_back(static_cast<char>(
                            packet->GetData<uint32_t>(offset, 1)));
                        offset += 1;
                    }

                    // Do something with the ascii message
                    ascii_messages.push_back(std::move(ascii));
                    break;
                }
                case (qchat::MessageTypes::Audio):
                {
                    Logger::Log(Logger::Level::Info, "offset ", offset);

                    Logger::Log(Logger::Level::Info, "Audio message len = ", (int)data_len);

                    uint8_t* audio_data = (packet->Data() + offset);

                    uint16_t* tx_buffer = audio.GetOutputBuffer();

                    AudioCodec::ALawExpand(audio_data, tx_buffer, data_len);

                    audio_data = nullptr;
                    tx_buffer = nullptr;

                    break;
                }
                default:
                {
                    break;
                }
            }
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

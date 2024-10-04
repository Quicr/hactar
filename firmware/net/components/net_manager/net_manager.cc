#include "net_manager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "serial_packet.hh"
#include "qchat.hh"

#include "esp_log.h"

#include <cstdint>
#include <vector>
#include <string>

typedef struct
{
    qchat::WatchRoom* watch;
    NetManager* manager;
} WatchRoomParams;

NetManager::NetManager(SerialPacketManager& ui_layer,
    Wifi& wifi
    // std::shared_ptr<QSession> qsession,
    // std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects
    )
    : ui_layer(ui_layer),
    wifi(wifi)
    // quicr_session(qsession),
    // inbound_objects(inbound_objects)
{
    xTaskCreate(HandleSerial, "handle_serial_task", 8192 * 2, (void*)this, 13, NULL);
    // xTaskCreate(HandleNetwork, "handle_network", 4096, (void*)this, 13, NULL);
}

void NetManager::Update()
{

}

void NetManager::HandleSerial(void* param)
{
    // TODO add a mutex
    NetManager* self = (NetManager*)param;

    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        self->ui_layer.Update(xTaskGetTickCount() / portTICK_PERIOD_MS);


        self->HandleSerialCommands();


        if (!self->ui_layer.HasRxPackets()) continue;

        auto& rx_packets = self->ui_layer.GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        Logger::Log(Logger::Level::Info, "Packet available");
        while (rx_packets.size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            std::unique_ptr<SerialPacket> rx_packet = std::move(rx_packets[0]);
            SerialPacket::Types packet_type = static_cast<SerialPacket::Types>(
                rx_packet->GetData<uint8_t>(0, 1));

            if (packet_type == SerialPacket::Types::QMessage)
            {
                Logger::Log(Logger::Level::Info, "NetManager: Handle for Packet:Type:Message");

                // Skip the packetId and go to the next part of the packet data
                uint8_t sub_message_type = rx_packet->GetData<uint8_t>(5, 1);
                // self->HandleQChatMessages(sub_message_type, rx_packet, 6);
            }

            rx_packets.pop_front();
        }
    }
}

// void NetManager::HandleQChatMessages(uint8_t message_type,
//     const std::unique_ptr<SerialPacket>& rx_packet,
//     const size_t offset)
// {
//     if (rx_packet == nullptr)
//     {
//         return;
//     }

//     if (quicr_session == nullptr)
//     {
//         Logger::Log(Logger::Level::Error, "Quicr session is null");
//         return;
//     }

//     if (!wifi.IsConnected())
//     {
//         // Not connected to wifi just skip

//         // Reject, and instead we need to reply with a NACK of sorts
//         return;
//     }

//     qchat::MessageTypes msg_type = static_cast<qchat::MessageTypes>(message_type);

//     switch (msg_type)
//     {
//         case qchat::MessageTypes::Watch:
//         {
//             Logger::Log(Logger::Level::Info, "Got an watch message");

//             qchat::WatchRoom* watch = new qchat::WatchRoom();
//             if (!qchat::Codec::decode(*watch, rx_packet, offset))
//             {
//                 Logger::Log(Logger::Level::Info, "NetManager:QChatMessage:Watch Decode  Failed");
//                 return;
//             }

//             WatchRoomParams* watch_params = new WatchRoomParams();
//             watch_params->watch = watch;
//             watch_params->manager = this;

//             xTaskCreate(HandleWatchMessage,
//                 "handle_watch_message",
//                 16384,
//                 (void*)watch_params,
//                 13,
//                 NULL);

//             break;
//         }
//         case qchat::MessageTypes::Ascii:
//         {
//             Logger::Log(Logger::Level::Info, "Got an ascii message");

//             qchat::Ascii ascii;
//             ascii.message.push_back((uint8_t)qchat::MessageTypes::Ascii);
//             bool result = qchat::Codec::decode(ascii, rx_packet, offset);
//             Logger::Log(Logger::Level::Debug, "Decoded an ascii message");

//             if (!result)
//             {
//                 Logger::Log(Logger::Level::Error, "NetManager:QChatMessage: Decode Ascii Message Failed");
//                 return;
//             }

//             auto nspace = quicr_session->to_namespace(ascii.message_uri);
//             // quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);

//             auto bytes = qchat::Codec::string_to_bytes(ascii.message);

//             Logger::Log(Logger::Level::Info, "Send to quicr session to publish");
//             quicr_session->publish(nspace, bytes);
//             break;
//         }
//         case qchat::MessageTypes::Audio:
//         {

//             Logger::Log(Logger::Level::Info, "Got an audio message");

//             // Get the room len
//             uint32_t room_len = rx_packet->GetData<uint32_t>(offset, 4);
//             uint32_t off = offset + 4;

//             // Get the room uri
//             std::string room_uri;
//             for (int i =0; i < room_len; ++i)
//             {
//                 room_uri.push_back(rx_packet->GetData<char>(off, 1));
//                 off += 1;
//             }

//             // Get the audio len
//             uint16_t audio_len = rx_packet->GetData<uint16_t>(off, 2);
//             off += 2;

//             std::vector<uint8_t> msg;
//             msg.reserve(audio_len + 1); // +1 for type
//             msg.push_back((uint8_t)qchat::MessageTypes::Audio);
//             Logger::Log(Logger::Level::Info, "Audio len ", audio_len);
//             for (uint16_t i = 0; i < audio_len; ++i)
//             {
//                 msg.push_back(rx_packet->GetData<uint8_t>(off, 1));
//                 off+=1;
//             }

//             auto nspace = quicr_session->to_namespace(room_uri);

//             Logger::Log(Logger::Level::Info, "Send audio to quicr session to publish");
//             quicr_session->publish(nspace, msg);
//             break;
//         }
//         default:
//         {
//             Logger::Log(Logger::Level::Warn, "NetManager:QChatMessage: Unknown Message");
//             break;
//         }
//     }
// }

// void NetManager::HandleWatchMessage(void* params)
// {
//     WatchRoomParams* watch_room_params = (WatchRoomParams*)params;
//     NetManager* self = watch_room_params->manager;
//     qchat::WatchRoom* watch = watch_room_params->watch;

//     Logger::Log(Logger::Level::Info, "Room URI: ", watch->room_uri.c_str());
//     Logger::Log(Logger::Level::Info, "Publisher URI: ", watch->publisher_uri.c_str());
//     try
//     {
//         if (!self->quicr_session->publish_intent(self->quicr_session->to_namespace(watch->room_uri)))
//         {
//             Logger::Log(Logger::Level::Error, "NetManager: QChatMessage:Watch publish_intent error");
//             return;
//         }

//         Logger::Log(Logger::Level::Debug, "Publish Intended");

//         if (!self->quicr_session->subscribe(self->quicr_session->to_namespace(watch->room_uri)))
//         {
//             Logger::Log(Logger::Level::Error, "NetManager: QChatMessage:Watch subscribe error");
//             return;
//         }

//         Logger::Log(Logger::Level::Debug, "Subscribed");

//         // why is this here?
//         vTaskDelay(2000 / portTICK_PERIOD_MS);

//         auto watch_ok_packet = std::make_unique<SerialPacket>(xTaskGetTickCount());
//         watch_ok_packet->SetData(SerialPacket::Types::QMessage, 0, 1);
//         watch_ok_packet->SetData(self->ui_layer.NextPacketId(), 1, 2);
//         watch_ok_packet->SetData(1, 3, 2);
//         watch_ok_packet->SetData(qchat::MessageTypes::WatchOk, 5, 1);

//         Logger::Log(Logger::Level::Debug, "Sending WatchOk to UI");
//         self->ui_layer.EnqueuePacket(std::move(watch_ok_packet));
//     }
//     catch (std::runtime_error& ex)
//     {
//         Logger::Log(Logger::Level::Error, "Failed to watch: ", ex.what());
//     }

//     delete watch_room_params->watch;
//     delete watch_room_params;

//     vTaskDelete(NULL);
// }

// void NetManager::HandleNetwork(void* param)
// {
//     // part 1, parse quicr message
//     // Get the quicr messages from the inbound objects
//     NetManager* self = (NetManager*)param;
//     while (true)
//     {
//         try
//         {
//             // TODO use a conditional variable
//             // Set the delay
//             vTaskDelay(50 / portTICK_PERIOD_MS);

//             if (self->inbound_objects->empty())
//             {
//                 continue;
//             }

//             const uint16_t Bytes_In_128_Bits = 128 / 8;

//             // Get the next item
//             auto qobj = self->inbound_objects->pop();
//             auto qname = qobj.name;
//             auto qdata = qobj.data;

//             // Create a net packet
//             std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>();

//             // Set the type
//             packet->SetData(SerialPacket::Types::QMessage, 0, 1);

//             // Set the id
//             packet->SetData(self->ui_layer.NextPacketId(), 1, 2);

//             // Set the length
//             uint16_t total_len = 10 + Bytes_In_128_Bits + qobj.data.size();
//             packet->SetData(total_len, 3, 2);

//             // Set the message type
//             packet->SetData((int)qchat::MessageTypes::Ascii, 5, 1);

//             // Set the length of the ascii message qname
//             packet->SetData(Bytes_In_128_Bits, 6, 4);

//             // Set the name, which is 128 bits.
//             size_t offset = 10;
//             for (size_t i = 0; i < Bytes_In_128_Bits; ++i)
//             {
//                 packet->SetData(qname[i], offset, 1);
//                 offset += 1;
//             }

//             // Set the length of the data
//             packet->SetData(qdata.size(), offset, 4);
//             offset += 4;

//             // Get the message from the item
//             for (size_t i = 0; i < qdata.size(); ++i)
//             {
//                 packet->SetData(qdata[i], offset, 1);
//                 offset += 1;
//             }

//             // Logger::Log(Logger::Level::Info, "Packet data total len %d", total_len);
//             // for (size_t i = 0; i < total_len; ++i)
//             // {
//             //     Logger::Log(Logger::Level::Info, (int)i, packet->GetData<int>(i, 1));
//             // }
//             // Logger::Log(Logger::Level::Info, "Enqueue serial packet that came from the network");
//             // Enqueue the packet to go to the UI
//             self->ui_layer.EnqueuePacket(std::move(packet));
//         }
//         catch (const std::exception& ex)
//         {
//             Logger::Log(Logger::Level::Info, "[HandleNetworkError]", ex.what());
//         }
//     }
// }

/**                          Private Functions                               **/
void NetManager::HandleSerialCommands()
{
    // Get the command type
    // uint8_t command_type = rx_packet->GetData<uint8_t>(5, 1);
    // Logger::Log(Logger::Level::Info, "Packet command received -", static_cast<int>(command_type));

    // switch (command_type)
    // {
        // case SerialPacket::Commands::SSIDs:
        //     GetSSIDsCommand();
        //     break;
        // case SerialPacket::Commands::WifiConnect:
        //     ConnectToWifiCommand(rx_packet);
        //     break;
        // case SerialPacket::Commands::WifiStatus:
        //     GetWifiStatusCommand();
        //     break;
        // case SerialPacket::Commands::RoomsGet:
        //     GetRoomsCommand();
        //     break;
        // default:
        //     break;
    // }

    // TODO move into wifi module
    RingBuffer<std::unique_ptr<SerialPacket>>* wifi_packets;

    if (!ui_layer.GetCommandPackets(&wifi_packets, SerialPacket::Commands::Wifi))
    {
        return;
    }

    while (wifi_packets->Unread() > 0)
    {
        Logger::Log(Logger::Level::Info, "Wifi packets available: ", wifi_packets->Unread());
        auto packet = std::move(wifi_packets->Read());
        // Get the type
        SerialPacket::WifiTypes wifi_cmd_type = static_cast<SerialPacket::WifiTypes>(
            packet->GetData<uint16_t>(7, 1));

        switch (wifi_cmd_type)
        {
            case SerialPacket::WifiTypes::Status:
            {
                GetWifiStatusCommand();
                break;
            }
            case SerialPacket::WifiTypes::SSIDs:
            {
                GetSSIDsCommand();
                break;
            }
            case SerialPacket::WifiTypes::Connect:
            {
                ConnectToWifiCommand(packet);
                break;
            }
            case SerialPacket::WifiTypes::Disconnect:
            {

                break;
            }
            case SerialPacket::WifiTypes::Disconnected:
            {

                break;
            }
            case SerialPacket::WifiTypes::SignalStrength:
            {

                break;
            }
            default:
            {
                break;
            }
        }
    }

    // TODO move this into some sort of qchat instance.
    RingBuffer<std::unique_ptr<SerialPacket>>* room_packets;

    if (!ui_layer.GetCommandPackets(&room_packets, SerialPacket::Commands::RoomsGet))
    {
        return;
    }

    while (room_packets->Unread() > 0)
    {
        Logger::Log(Logger::Level::Info, "room packets available: ", room_packets->Unread());
        auto packet = std::move(room_packets->Read());

        GetRoomsCommand();
    }
}

void NetManager::GetSSIDsCommand()
{
    // Get the ssids
    std::vector<std::string> ssids;
    esp_err_t res = wifi.ScanNetworks(&ssids);

    // ERROR Here for some reason...
    if (res != ESP_OK)
    {
        Logger::Log(Logger::Level::Error, "Error while scanning networks");
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return;
    }

    if (ssids.size() == 0)
    {
        Logger::Log(Logger::Level::Warn, "No networks found");
        return;
    }

    Logger::Log(Logger::Level::Info, "SSIDs found -", ssids.size());

    // Put ssids into a vector of packets and enqueue them
    for (uint16_t i = 0; i < ssids.size(); ++i)
    {
        std::string& ssid = ssids[i];
        if (ssid.length() == 0) continue;
        Logger::Log(Logger::Level::Info, i, "length -", ssid.length());

        std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>();

        // Set the type
        packet->SetData(SerialPacket::Types::Command, 0, 1);

        // Set the packet id
        packet->SetData(ui_layer.NextPacketId(), 1, 2);

        // + 2 for command type
        // + 1 for wifi type
        // + 1 for the ssid id
        // + 1 for ssid len
        packet->SetData(ssid.length() + 5, 3, 2);

        // Set the first byte to the command type
        packet->SetData(SerialPacket::Commands::Wifi, 5, 2);
        packet->SetData(SerialPacket::WifiTypes::SSIDs, 7, 1);

        // Set the ssid id
        packet->SetData(i + 1, 8, 1);

        // Set the string length
        packet->SetData(ssid.length(), 9, 1);

        // Add each character of the string to the packet
        for (uint16_t j = 0; j < ssid.length(); j++)
        {
            packet->SetData(ssid[j], 10 + j, 1);
        }
        ui_layer.EnqueuePacket(std::move(packet));
    }
}


void NetManager::ConnectToWifiCommand(const std::unique_ptr<SerialPacket>& packet)
{        // Get the ssid value, followed by the ssid_password
    uint16_t ssid_len = packet->GetData<uint16_t>(8, 2);
    Logger::Log(Logger::Level::Info, "SSID length -", ssid_len);

    // Build the ssid
    std::string ssid;
    unsigned short offset = 10;
    for (uint16_t i = 0; i < ssid_len; ++i, offset += 1)
    {
        ssid += packet->GetData<char>(offset, 1);
    }
    Logger::Log(Logger::Level::Info, "SSID -", ssid.c_str());

    uint16_t ssid_password_len = packet->GetData<uint16_t>(offset, 2);
    offset += 2;
    Logger::Log(Logger::Level::Info, "Password length -", ssid_password_len);

    std::string ssid_password;
    for (uint16_t j = 0; j < ssid_password_len; ++j, offset += 1)
    {
        ssid_password += packet->GetData<char>(offset, 1);
    }
    Logger::Log(Logger::Level::Info, "SSID Password -", ssid_password.c_str());

    wifi.Connect(ssid.c_str(), ssid_password.c_str());
}

void NetManager::GetWifiStatusCommand()
{
    Logger::Log(Logger::Level::Info, "Connection status -", static_cast<int>(wifi.GetState()));

    // Create a packet that tells the current status
    std::unique_ptr<SerialPacket> connected_packet = std::make_unique<SerialPacket>();
    connected_packet->SetData(SerialPacket::Types::Command, 0, 1);
    connected_packet->SetData(ui_layer.NextPacketId(), 1, 2);
    connected_packet->SetData(4, 3, 2);
    connected_packet->SetData(SerialPacket::Commands::Wifi, 5, 2);
    connected_packet->SetData((int)SerialPacket::WifiTypes::Status, 7, 1);
    connected_packet->SetData(wifi.GetState() == Wifi::State::Connected, 8, 1);

    ui_layer.EnqueuePacket(std::move(connected_packet));

}

static qchat::Room CreateFakeRoom()
{
    return qchat::Room(
        true,
        "CAFE",
        "quicr://webex.cisco.com/version/1/appId/1/org/1/channel/100/room/1/",
        "quicr://webex.cisco.com/version/1/appId/1/org/1/channel/100/room/1",
        "root"
    );
}

void NetManager::GetRoomsCommand()
{
    // TODO real function for now just make a fake room...

    // Fake room
    Logger::Log(Logger::Level::Debug, "Send fake room");

    std::unique_ptr<SerialPacket> room_packet = std::make_unique<SerialPacket>();
    room_packet->SetData(SerialPacket::Types::Command, 0, 1);
    room_packet->SetData(ui_layer.NextPacketId(), 1, 2);
    qchat::Codec::encode(room_packet, CreateFakeRoom());

    ui_layer.EnqueuePacket(std::move(room_packet));
}
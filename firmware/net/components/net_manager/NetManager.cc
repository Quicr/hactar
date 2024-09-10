#include "NetManager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "SerialPacket.hh"
#include "QChat.hh"

#include <transport/transport.h>
#include "esp_log.h"

#include <cstdint>
#include <vector>
#include <string>

typedef struct
{
    qchat::WatchRoom* watch;
    NetManager* manager;
} WatchRoomParams;

NetManager::NetManager(SerialPacketManager* _ui_layer,
    std::shared_ptr<QSession> qsession,
    std::shared_ptr<AsyncQueue<QuicrObject>> inbound_objects)
    : ui_layer(_ui_layer),
    quicr_session(qsession),
    inbound_objects(inbound_objects)
{

    xTaskCreate(HandleSerial, "handle_serial_task", 8192 * 2, (void*)this, 13, NULL);
    xTaskCreate(HandleNetwork, "handle_network", 4096, (void*)this, 13, NULL);
}

void NetManager::HandleSerial(void* param)
{
    // TODO add a mutex
    NetManager* self = (NetManager*)param;

    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        self->ui_layer->RxTx(xTaskGetTickCount() / portTICK_PERIOD_MS);

        if (!self->ui_layer->HasRxPackets()) continue;

        auto& rx_packets = self->ui_layer->GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        Logger::Log(Logger::Level::Info, "Packet available");
        while (rx_packets.size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            std::unique_ptr<SerialPacket> rx_packet = std::move(rx_packets[0]);
            uint8_t packet_type = rx_packet->GetData<uint8_t>(0, 1);

            if (packet_type == SerialPacket::Types::Message)
            {
                Logger::Log(Logger::Level::Info, "NetManager: Handle for Packet:Type:Message");

                // skip the packetId and go to the next part of the packet data
                // 6 + 8 = 14, skip to 14,
                // Mesages have sub-types whuch is encoded in the first byte
                uint8_t sub_message_type = rx_packet->GetData<uint8_t>(5, 1);
                self->HandleQChatMessages(sub_message_type, rx_packet, 6);
            }
            else if (packet_type == SerialPacket::Types::Command)
            {
                self->HandleSerialCommands(rx_packet);
            }

            rx_packets.pop_front();
        }
    }
}

void NetManager::HandleQChatMessages(uint8_t message_type,
    const std::unique_ptr<SerialPacket>& rx_packet,
    const size_t offset)
{
    if (rx_packet == nullptr)
    {
        return;
    }

    if (quicr_session == nullptr)
    {
        Logger::Log(Logger::Level::Error, "Quicr session is null");
        return;
    }

    if (!Wifi::GetInstance()->IsConnected())
    {
        // Not connected to wifi just skip

        // Reject, and instead we need to reply with a NACK of sorts
        return;
    }

    qchat::MessageTypes msg_type = static_cast<qchat::MessageTypes>(message_type);

    switch (msg_type)
    {
        case qchat::MessageTypes::Watch:
        {
            Logger::Log(Logger::Level::Info, "Got an watch message");

            qchat::WatchRoom* watch = new qchat::WatchRoom();
            if (!qchat::Codec::decode(*watch, rx_packet, offset))
            {
                Logger::Log(Logger::Level::Info, "NetManager:QChatMessage:Watch Decode  Failed");
                return;
            }

            WatchRoomParams* watch_params = new WatchRoomParams();
            watch_params->watch = watch;
            watch_params->manager = this;

            xTaskCreate(HandleWatchMessage,
                "handle_watch_message",
                16384,
                (void*)watch_params,
                13,
                NULL);

            break;
        }
        case qchat::MessageTypes::Ascii:
        {
            Logger::Log(Logger::Level::Info, "Got an ascii message");

            qchat::Ascii ascii;
            bool result = qchat::Codec::decode(ascii, rx_packet, offset);
            Logger::Log(Logger::Level::Debug, "Decoded an ascii message");

            if (!result)
            {
                Logger::Log(Logger::Level::Error, "NetManager:QChatMessage: Decode Ascii Message Failed");
                return;
            }

            auto nspace = quicr_session->to_namespace(ascii.message_uri);
            // quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);

            auto bytes = qchat::Codec::string_to_bytes(ascii.message);

            Logger::Log(Logger::Level::Info, "Send to quicr session to publish");
            quicr_session->publish(nspace, bytes);
            break;
        }
        default:
        {
            Logger::Log(Logger::Level::Warn, "NetManager:QChatMessage: Unknown Message");
            break;
        }
    }
}

void NetManager::HandleWatchMessage(void* params)
{
    WatchRoomParams* watch_room_params = (WatchRoomParams*)params;
    NetManager* self = watch_room_params->manager;
    qchat::WatchRoom* watch = watch_room_params->watch;

    Logger::Log(Logger::Level::Info, "Room URI: ", watch->room_uri.c_str());
    Logger::Log(Logger::Level::Info, "Publisher URI: ", watch->publisher_uri.c_str());
    try
    {
        if (!self->quicr_session->publish_intent(self->quicr_session->to_namespace(watch->room_uri)))
        {
            Logger::Log(Logger::Level::Error, "NetManager: QChatMessage:Watch publish_intent error");
            return;
        }

        Logger::Log(Logger::Level::Debug, "Publish Intended");

        if (!self->quicr_session->subscribe(self->quicr_session->to_namespace(watch->room_uri)))
        {
            Logger::Log(Logger::Level::Error, "NetManager: QChatMessage:Watch subscribe error");
            return;
        }

        Logger::Log(Logger::Level::Debug, "Subscribed");

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        auto watch_ok_packet = std::make_unique<SerialPacket>(xTaskGetTickCount());
        watch_ok_packet->SetData(SerialPacket::Types::Message, 0, 1);
        watch_ok_packet->SetData(self->ui_layer->NextPacketId(), 1, 2);
        watch_ok_packet->SetData(1, 3, 2);
        watch_ok_packet->SetData(qchat::MessageTypes::WatchOk, 5, 1);

        Logger::Log(Logger::Level::Debug, "Sending WatchOk to UI");
        self->ui_layer->EnqueuePacket(std::move(watch_ok_packet));
    }
    catch (std::runtime_error& ex)
    {
        Logger::Log(Logger::Level::Error, "Failed to watch: ", ex.what());
    }

    delete watch_room_params->watch;
    delete watch_room_params;

    vTaskDelete(NULL);
}

void NetManager::HandleNetwork(void* param)
{
    // part 1, parse quicr message
    // Get the quicr messages from the inbound objects
    NetManager* self = (NetManager*)param;
    while (true)
    {
        try
        {

            // Set the delay
            vTaskDelay(50 / portTICK_PERIOD_MS);

            if (self->inbound_objects->empty())
            {
                continue;
            }

            const uint16_t Bytes_In_128_Bits = 128 / 8;

            // Get the next item
            auto qobj = self->inbound_objects->pop();
            auto qname = qobj.name;
            auto qdata = qobj.data;

            // Create a net packet
            std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>();

            // Set the type
            packet->SetData(SerialPacket::Types::Message, 0, 1);

            // Set the id
            packet->SetData(0, 1, 2);

            // Set the length
            uint16_t total_len = 9 + Bytes_In_128_Bits + qobj.data.size();
            packet->SetData(total_len, 3, 2);

            // Set the message type
            packet->SetData((int)qchat::MessageTypes::Ascii, 5, 1);

            // Set the length of the ascii message qname
            packet->SetData(Bytes_In_128_Bits, 6, 4);

            // Set the name, which is 128 bits.
            size_t offset = 10;
            for (size_t i = 0; i < Bytes_In_128_Bits; ++i)
            {
                packet->SetData(qname[i], offset, 1);
                offset += 1;
            }

            // Set the length of the data
            packet->SetData(qdata.size(), offset, 4);
            offset += 4;

            // Get the message from the item
            for (size_t i = 0; i < qdata.size(); ++i)
            {
                packet->SetData(qdata[i], offset, 1);
                offset += 1;
            }

            Logger::Log(Logger::Level::Debug, "Packet data total len %d", total_len);
            Logger::Log(Logger::Level::Info, "Enqueue serial packet that came from the network");

            // Enqueue the packet to go to the UI
            self->ui_layer->EnqueuePacket(std::move(packet));
            packet = nullptr;
        }
        catch (const std::exception& ex)
        {
            Logger::Log(Logger::Level::Info, "[HandleNetworkError]", ex.what());
        }
    }

    //Ascii ascii;
    // part2 encode into qchat message and send to ui chip
}

/**                          Private Functions                               **/
void NetManager::HandleSerialCommands(const std::unique_ptr<SerialPacket>& rx_packet)
{
    // Get the command type
    uint8_t command_type = rx_packet->GetData<uint8_t>(5, 1);
    Logger::Log(Logger::Level::Info, "Packet command received -", static_cast<int>(command_type));

    switch (command_type)
    {
        case SerialPacket::Commands::SSIDs:
            GetSSIDsCommand();
            break;
        case SerialPacket::Commands::WifiConnect:
            ConnectToWifiCommand(rx_packet);
            break;
        case SerialPacket::Commands::WifiStatus:
            GetWifiStatusCommand();
            break;
        case SerialPacket::Commands::RoomsGet:
            GetRoomsCommand();
            break;
        default:
            break;
    }
}

void NetManager::GetSSIDsCommand()
{
    // Get the ssids
    std::vector<std::string> ssids;
    esp_err_t res = Wifi::GetInstance()->ScanNetworks(&ssids);

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
        packet->SetData(ui_layer->NextPacketId(), 1, 2);

        // Add 1 for the command type
        // Add 1 for the ssid id
        packet->SetData(ssid.length() + 2, 3, 2);

        // Set the first byte to the command type
        packet->SetData(SerialPacket::Commands::SSIDs, 5, 1);

        // Set the ssid id
        packet->SetData(i + 1, 6, 1);

        // Add each character of the string to the packet
        for (uint16_t j = 0; j < ssid.length(); j++)
        {
            packet->SetData(ssid[j], 7 + j, 1);
        }
        ui_layer->EnqueuePacket(std::move(packet));
    }
}


void NetManager::ConnectToWifiCommand(const std::unique_ptr<SerialPacket>& packet)
{        // Get the ssid value, followed by the ssid_password
    uint16_t ssid_len = packet->GetData<uint16_t>(6, 2);
    Logger::Log(Logger::Level::Info, "SSID length -", ssid_len);

    // Build the ssid
    std::string ssid;
    unsigned short offset = 8;
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

    Wifi::GetInstance()->Connect(ssid.c_str(), ssid_password.c_str());
}

void NetManager::GetWifiStatusCommand()
{
    auto wifi = Wifi::GetInstance();
    Logger::Log(Logger::Level::Info, "Connection status -", static_cast<int>(wifi->GetState()));

    // Create a packet that tells the current status
    std::unique_ptr<SerialPacket> connected_packet = std::make_unique<SerialPacket>();
    connected_packet->SetData(SerialPacket::Types::Command, 0, 1);
    connected_packet->SetData(ui_layer->NextPacketId(), 1, 2);
    connected_packet->SetData(2, 3, 2);
    connected_packet->SetData(SerialPacket::Commands::WifiStatus, 5, 1);
    connected_packet->SetData(wifi->GetState() == Wifi::State::Connected, 6, 1);

    ui_layer->EnqueuePacket(std::move(connected_packet));

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
    room_packet->SetData(ui_layer->NextPacketId(), 1, 2);
    qchat::Codec::encode(room_packet, CreateFakeRoom());

    ui_layer->EnqueuePacket(std::move(room_packet));
}
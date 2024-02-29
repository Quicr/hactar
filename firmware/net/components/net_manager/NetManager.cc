#include "NetManager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "SerialPacket.hh"
#include "Vector.hh"
#include "String.hh"
#include "QChat.hh"

#include <transport/transport.h>
#include "esp_log.h"

typedef struct
{
    qchat::WatchRoom* watch;
    NetManager* manager;
} WatchRoomParams;

static const char* TAG = "[Net Manager]: ";

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

    // Vector<std::unique_ptr<Packet>>* rx_packets = nullptr;
    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        self->ui_layer->RxTx(xTaskGetTickCount() / portTICK_PERIOD_MS);

        if (!self->ui_layer->HasRxPackets()) continue;

        const Vector<std::unique_ptr<SerialPacket>>& rx_packets = self->ui_layer->GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        printf("%s\r\n", "NetManager: Packet available");
        while (rx_packets.size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            std::unique_ptr<SerialPacket> rx_packet = std::move(rx_packets[0]);
            uint8_t packet_type = rx_packet->GetData<uint8_t>(0, 1);
            uint16_t data_len = rx_packet->GetData<uint16_t>(3, 2);

            // uint8_t data;
            // printf("NET: Message from ui chip - ");
            // for (uint16_t i = 0; i < data_len; ++i)
            // {
            //     // Get each data byte
            //     data = rx_packet->GetData(24 + (i * 8), 8);
            //     if ((data >= '0' && data <= '9') || (data >= 'A' && data <= 'z'))
            //     {
            //         printf("%c", (char)data);
            //     }
            //     else
            //     {
            //         printf("%d ", (int)data);
            //     }
            // }
            // printf("\n\r");

            if (packet_type == SerialPacket::Types::Message)
            {
                printf("%s\r\n", "NetManager: Handle for Packet:Type:Message");
                // skip the packetId and go to the next part of the packet data
                // 6 + 8 = 14, skip to 14,
                // Mesages have sub-types whuch is encoded in the first byte
                uint8_t sub_message_type = rx_packet->GetData<uint8_t>(5, 1);
                    // qchat message handler
                    //handle_qchat_message(sub_message_type, rx_packet);
                    self->HandleQChatMessages(sub_message_type, rx_packet, 6);

            }
            else if (packet_type == SerialPacket::Types::Command)
            {
                self->HandleSerialCommands(rx_packet);
            }

            self->ui_layer->DestroyRxPacket(0);
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
        ESP_LOGE(TAG, "Quicr session is null");
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
            ESP_LOGI(TAG, "Got an watch message");

            qchat::WatchRoom* watch = new qchat::WatchRoom();
            if (!qchat::Codec::decode(*watch, rx_packet, offset))
            {
                printf("%s\r\n", "NetManager:QChatMessage:Watch Decode  Failed");
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
            ESP_LOGI(TAG, "Got an ascii message");

            qchat::Ascii ascii;
            bool result = qchat::Codec::decode(ascii, rx_packet, offset);
            ESP_LOGI(TAG, "Decoded an ascii message");
            if (!result)
            {
                printf("%s\r\n", "NetManager:QChatMessage: Decode Ascii Message Failed");
                return;
            }

            auto nspace = quicr_session->to_namespace(ascii.message_uri);
            // quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);

            auto bytes = qchat::Codec::string_to_bytes(ascii.message);

            Logger::Log("Send to quicr session to publish");
            quicr_session->publish(nspace, bytes);
            break;
        }
        default:
        {
            printf("%s\r\n", "NetManager:QChatMessage: Unknown Message");
            break;
        }
    }
}

void NetManager::HandleWatchMessage(void* params)
{
    WatchRoomParams* watch_room_params = (WatchRoomParams*)params;
    NetManager* self = watch_room_params->manager;
    qchat::WatchRoom* watch = watch_room_params->watch;

    Logger::Log("Room URI: ", watch->room_uri.c_str());
    Logger::Log("Publisher URI: ", watch->publisher_uri.c_str());
    try
    {
        if (!self->quicr_session->publish_intent(self->quicr_session->to_namespace(watch->room_uri)))
        {
            Logger::Log("NetManager: QChatMessage:Watch publish_intent error ");
            return;
        }

        Logger::Log("Publish Intended");

        if (!self->quicr_session->subscribe(self->quicr_session->to_namespace(watch->room_uri)))
        {
            Logger::Log("NetManager: QChatMessage:Watch subscribe error ");
            return;
        }

        Logger::Log("Subscribed");

        vTaskDelay(2000 / portTICK_PERIOD_MS);

        auto watch_ok_packet = std::make_unique<SerialPacket>(xTaskGetTickCount());
        watch_ok_packet->SetData(SerialPacket::Types::Message, 0, 1);
        watch_ok_packet->SetData(self->ui_layer->NextPacketId(), 1, 2);
        watch_ok_packet->SetData(1, 3, 2);
        watch_ok_packet->SetData(qchat::MessageTypes::WatchOk, 5, 1);

        Logger::Log("Sending WatchOk to UI");
        self->ui_layer->EnqueuePacket(std::move(watch_ok_packet));
    }
    catch (std::runtime_error& ex)
    {
        Logger::Log("Failed to watch: ", ex.what());
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

            ESP_LOGI(TAG, "Packet data total len %d", total_len);
            for (size_t i = 0; i < total_len + 3; ++i)
            {
                printf("%d\r\n", packet->GetData<int>(i, 1));
            }
            ESP_LOGI(TAG, "Enqueue serial packet that came from the network");
            // Enqueue the packet to go to the UI
            self->ui_layer->EnqueuePacket(std::move(packet));
            packet = nullptr;
        }
        catch (const std::exception& ex)
        {
            printf("[HandleNetworkError] %s\n", ex.what());
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
    printf("NET: Packet command received - %d\n\r", (int)command_type);

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
    Vector<String> ssids;
    esp_err_t res = Wifi::GetInstance()->ScanNetworks(&ssids);

    // ERROR Here for some reason...
    if (res != ESP_OK)
    {
        printf("Error while scanning networks");
        ESP_ERROR_CHECK_WITHOUT_ABORT(res);
        return;
    }

    if (ssids.size() == 0)
    {
        printf("No networks found\n\r");
        return;
    }

    printf("SSIDs found - %d\n\r", ssids.size());

    // Put ssids into a vector of packets and enqueue them
    for (uint16_t i = 0; i < ssids.size(); ++i)
    {
        String& ssid = ssids[i];
        if (ssid.length() == 0) continue;
        printf("%d. length - %d\n\r", i, ssid.length());

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
    printf("NET SSID length - %d\n\r", ssid_len);

    // Build the ssid
    String ssid;
    unsigned short offset = 8;
    for (uint16_t i = 0; i < ssid_len; ++i)
    {
        ssid += packet->GetData<char>(offset, 1);
        offset += 1;
    }
    printf("NET: SSID - %s\r\n", ssid.c_str());

    uint16_t ssid_password_len = packet->GetData<uint16_t>(offset, 2);
    offset += 2;
    printf("NET: Password length - %d\n\r", ssid_password_len);

    String ssid_password;
    for (uint16_t j = 0; j < ssid_password_len; ++j)
    {
        ssid_password += packet->GetData<char>(offset, 1);
        offset += 1;
    }
    printf("NET: SSID Password - %s\n\r", ssid_password.c_str());

    Wifi::GetInstance()->Connect(ssid.c_str(), ssid_password.c_str());
}

void NetManager::GetWifiStatusCommand()
{
    auto wifi = Wifi::GetInstance();
    printf("NET: Connection status - %d\n\r", (int)wifi->GetState());

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
    ESP_LOGI(TAG, "Send fake room");

    std::unique_ptr<SerialPacket> room_packet = std::make_unique<SerialPacket>();
    room_packet->SetData(SerialPacket::Types::Command, 0, 1);
    room_packet->SetData(ui_layer->NextPacketId(), 1, 2);
    qchat::Codec::encode(room_packet, CreateFakeRoom());

    ui_layer->EnqueuePacket(std::move(room_packet));
}
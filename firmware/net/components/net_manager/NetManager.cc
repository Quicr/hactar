#include "NetManager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Packet.hh"
#include "Vector.hh"
#include "String.hh"
#include "QChat.hh"

#include <transport/transport.h>
#include "esp_log.h"

typedef struct
{
    qchat::WatchRoom* watch;
    NetManager* manager;
}WatchRoomParams;

static const char* TAG = "[Net Manager]: ";

NetManager::NetManager(SerialManager* _ui_layer,
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
    NetManager* _this = (NetManager*)param;

    // Vector<Packet*>* rx_packets = nullptr;
    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        _this->ui_layer->RxTx(xTaskGetTickCount() / portTICK_PERIOD_MS);

        if (!_this->ui_layer->HasRxPackets()) continue;

        const Vector<Packet*>& rx_packets = _this->ui_layer->GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        while (rx_packets.size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            Packet* rx_packet = rx_packets[0];
            uint8_t packet_type = rx_packet->GetData(0, 6);
            uint16_t data_len = rx_packet->GetData(14, 10);
            uint8_t data;
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

            if (packet_type == Packet::Types::Message)
            {
                printf("%s\r\n", "NetManager: Handle for Packet:Type:Message");
                // skip the packetId and go to the next part of the packet data
                // 6 + 8 = 14, skip to 14,
                // Mesages have sub-types whuch is encoded in the first byte
                uint8_t sub_message_type = rx_packet->GetData(24, 8);
                if (sub_message_type >= 0 && sub_message_type <= 2)
                {
                    // qchat message handler
                    //handle_qchat_message(sub_message_type, rx_packet);
                    _this->HandleQChatMessages(sub_message_type, rx_packet, 32);
                }

            }
            else if (packet_type == Packet::Types::Command)
            {
                _this->HandleSerialCommands(rx_packet);
            }

            _this->ui_layer->DestroyRxPacket(0);
        }
    }
}

void NetManager::HandleQChatMessages(uint8_t message_type, Packet* rx_packet, size_t offset)
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
            if (!result)
            {
                printf("%s\r\n", "NetManager:QChatMessage: Decode Ascii Message Failed");
                return;
            }

            auto nspace = quicr_session->to_namespace(ascii.message_uri);
            // quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);

            auto bytes = qchat::Codec::string_to_bytes(ascii.message);

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
    NetManager* _this = watch_room_params->manager;
    qchat::WatchRoom* watch = watch_room_params->watch;

    printf("[net] room uri %s\n", watch->room_uri.c_str());
    printf("[net] publisher uri %s\n", watch->publisher_uri.c_str());
    try
    {
        // quicr::Namespace sub_ns = quicr_session->to_namespace(watch->room_uri);

        if (!_this->quicr_session->subscribe(_this->quicr_session->to_namespace(watch->room_uri)))
        {
            printf("%s\r\n", "NetManager:QChatMessage:Watch subscribe error ");
            return;
        }

        ESP_LOGI(TAG, "Subscribed");

        // TODO ask Suhas about this
        // if (!_this->quicr_session->publish_intent(_this->quicr_session->to_namespace(watch->publisher_uri)))
        if (!_this->quicr_session->publish_intent(_this->quicr_session->to_namespace(watch->room_uri)))
        {
            printf("%s\r\n", "NetManager:QChatMessage:Watch publish_intent error ");
            return;
        }
    }
    catch (std::runtime_error& ex)
    {
        printf("[net] failed to watch, %s\n", ex.what());
    }

    delete watch_room_params->watch;
    delete watch_room_params;

    vTaskDelete(NULL);
}

void NetManager::HandleNetwork(void* param)
{
    // part 1, parse quicr message
    // Get the quicr messages from the inbound objects
    NetManager* _this = (NetManager*)param;
    while (true)
    {
        try
        {

            // Set the delay
            vTaskDelay(50 / portTICK_PERIOD_MS);

            if (_this->inbound_objects->empty())
            {
                continue;
            }

            const uint16_t bytes_in_128_bits = 128 / 8;

            // Get the next item
            auto qobj = _this->inbound_objects->pop();
            auto qname = qobj.name;
            auto qdata = qobj.data;

            // Create a net packet
            Packet* packet = new Packet();

            // Set the type
            packet->SetData(Packet::Types::Message, 0, 6);

            // Set the id
            packet->SetData(0, 6, 8);

            // Set the length
            uint16_t total_len = 9 + bytes_in_128_bits + qobj.data.size();
            packet->SetData(total_len, 14, 10);

            // Set the message type
            packet->SetData((int)qchat::MessageTypes::Ascii, 24, 8);

            // Set the length of the ascii message qname
            packet->SetData(bytes_in_128_bits, 32, 32);

            // Set the name, which is 128 bits.
            size_t offset = 64;
            for (size_t i = 0; i < bytes_in_128_bits; ++i)
            {
                packet->SetData(qname[i], offset, 8);
                offset += 8;
            }

            // Set the length of the data
            packet->SetData(qdata.size(), offset, 32);
            offset += 32;

            // Get the message from the item
            for (size_t i = 0; i < qdata.size(); ++i)
            {
                packet->SetData(qdata[i], offset, 8);
                offset += 8;
            }

            ESP_LOGI(TAG, "Packet data total len %d", total_len);
            for (size_t i = 0; i < total_len + 3; ++i)
            {
                printf("%d\r\n", (int)packet->GetData(i * 8, 8));
            }
            ESP_LOGI(TAG, "Enqueue serial packet that came from the network");
            // Enqueue the packet to go to the UI
            _this->ui_layer->EnqueuePacket(packet);
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
void NetManager::HandleSerialCommands(Packet* rx_packet)
{
    // Get the command type
    uint8_t command_type = rx_packet->GetData(24, 8);
    printf("NET: Packet command received - %d\n\r", (int)command_type);
    if (command_type == Packet::Commands::SSIDs)
    {
        // Get the ssids
        Vector<String> ssids;
        esp_err_t res = Wifi::GetInstance()->ScanNetworks(&ssids);

        if (res != ESP_OK)
        {
            printf("Error while scanning networks");
            ESP_ERROR_CHECK(res);
            return;
        }

        if (ssids.size() == 0)
        {
            printf("No networks found\n\r");
            return;
        }

        printf("SSIDs found - %d\n\r", ssids.size());

        // Put ssids into a vector of packets and enqueue them
        for (unsigned int i = 0; i < ssids.size(); ++i)
        {
            String& ssid = ssids[i];
            if (ssid.length() == 0) continue;
            printf("%d. length - %d\n\r", i, ssid.length());

            Packet* packet = new Packet();

            // Set the type
            packet->SetData(Packet::Types::Command, 0, 6);

            // Set the packet id
            packet->SetData(1, 6, 8);

            // Add 1 for the command type
            // Add 1 for the ssid id
            packet->SetData(ssid.length() + 2, 14, 10);

            // Set the first byte to the command type
            packet->SetData(Packet::Commands::SSIDs, 24, 8);

            // Set the ssid id
            packet->SetData(i + 1, 32, 8);

            // Add each character of the string to the packet
            for (unsigned int j = 0; j < ssid.length(); j++)
            {
                packet->SetData(ssid[j], 40 + (j * 8), 8);
            }
            ui_layer->EnqueuePacket(packet);
        }
    }
    else if (command_type == Packet::Commands::ConnectToSSID)
    {
        // Get the ssid value, followed by the ssid_password
        unsigned char ssid_len = rx_packet->GetData(32, 8);
        printf("NET SSID length - %d\n\r", ssid_len);

        // Build the ssid
        String ssid;
        unsigned short offset = 40;
        unsigned char i = 0;
        for (i = 0; i < ssid_len; ++i)
        {
            ssid += static_cast<char>(
                rx_packet->GetData(offset, 8));
            offset += 8;
        }
        printf("NET: SSID - %s\r\n", ssid.c_str());

        unsigned char ssid_password_len = rx_packet->GetData(offset, 8);
        offset += 8;
        printf("NET: Password length - %d\n\r", ssid_password_len);

        String ssid_password;
        for (unsigned char j = 0; j < ssid_password_len; ++j)
        {
            ssid_password += static_cast<char>(rx_packet->GetData(
                offset, 8));
            offset += 8;
        }
        printf("NET: SSID Password - %s\n\r", ssid_password.c_str());

        Wifi::GetInstance()->Connect(ssid.c_str(), ssid_password.c_str());
    }
    else if (command_type == Packet::Commands::WifiStatus)
    {
        auto wifi = Wifi::GetInstance();
        printf("NET: Connection status - %d\n\r", (int)wifi->GetState());

        // Create a packet that tells the current status
        Packet* connected_packet = new Packet();
        connected_packet->SetData(Packet::Types::Command, 0, 6);
        connected_packet->SetData(1, 6, 8);
        connected_packet->SetData(2, 14, 10);
        connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        connected_packet->SetData(wifi->GetState() == Wifi::State::Connected, 32, 8);

        ui_layer->EnqueuePacket(connected_packet);
    }
}

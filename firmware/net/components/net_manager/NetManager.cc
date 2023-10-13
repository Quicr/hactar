#include "NetManager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Packet.hh"
#include "Vector.hh"
#include "String.hh"

#include <transport/transport.h>
#include <quicr/quicr_client_common.h>

NetManager::NetManager(SerialManager* _ui_layer)
    : ui_layer(_ui_layer)
{
    // Start tasks??
    wifi = hactar_utils::Wifi::GetInstance();
    esp_err_t res = wifi->Initialize();
    ESP_ERROR_CHECK(res);
    char default_relay [] = "192.168.50.19";
    auto relay_name = default_relay;
    uint16_t port = 33434;

    quicr::RelayInfo relay{
        .hostname = relay_name,
            .port = port,
            .proto = quicr::RelayInfo::Protocol::UDP
    };
    qsession = new QSession(relay);
    xTaskCreate(HandleSerial, "handle_serial_task", 4096, (void*)this, 13, NULL);
}

void NetManager::HandleSerial(void* param)
{
    // TODO add a mutex
    NetManager* _this = (NetManager*)param;

    Vector<Packet*>* rx_packets = nullptr;
    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        _this->ui_layer->RxTx(xTaskGetTickCount() / portTICK_PERIOD_MS);

        if (!_this->ui_layer->HasRxPackets()) continue;

        rx_packets = &_this->ui_layer->GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        while (rx_packets->size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            printf("NET: Message from ui chip - ");
            Packet* rx_packet = (*rx_packets)[0];
            uint8_t packet_type = rx_packet->GetData(0, 6);
            uint16_t data_len = rx_packet->GetData(14, 10);

            uint8_t data;
            for (uint16_t i = 0; i < data_len; ++i)
            {
                // Get each data byte
                data = rx_packet->GetData(24 + (i * 8), 8);
                if ((data >= '0' && data <= '9') || (data >= 'A' && data <= 'z'))
                {
                    printf("%c", (char)data);
                }
                else
                {
                    printf("%d ", (int)data);
                }
            }
            printf("\n\r");

            if (packet_type == Packet::Types::Message)
            {
                printf("%s\r\n", "I am here");
                // Pass the message to the next layer of processing...
                // aka quicr
                // 1. if it is watch message
                //_this->qsession->subscribe(publish_uri, payload)
                // ascii
                //_this->qsession->publish(publish_ur, payload)
                
            }
            else if (packet_type == Packet::Types::Command)
            {
                _this->HandleSerialCommands(rx_packet);
            }

            delete rx_packet;
            rx_packet = nullptr;
            rx_packets->erase(0);
        }
    }
}

void NetManager::HandleNetwork(void* param)
{
    // TODO
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
        esp_err_t res = wifi->ScanNetworks(&ssids);

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

        wifi->Connect(ssid.c_str(), ssid_password.c_str());
    }
    else if (command_type == Packet::Commands::WifiStatus)
    {
        printf("NET: Connection status - %d\n\r", (int)wifi->IsConnected());

        // Create a packet that tells the current status
        Packet* connected_packet = new Packet();
        connected_packet->SetData(Packet::Types::Command, 0, 6);
        connected_packet->SetData(1, 6, 8);
        connected_packet->SetData(2, 14, 10);
        connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        connected_packet->SetData(wifi->IsConnected(), 32, 8);

        ui_layer->EnqueuePacket(connected_packet);
    }
}

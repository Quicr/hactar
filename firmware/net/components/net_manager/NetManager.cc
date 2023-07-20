#include "NetManager.hh"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Packet.hh"
#include "Vector.hh"
#include "String.hh"

NetManager::NetManager(SerialManager* serial)
    : serial(serial)
{
    // Start tasks??

    xTaskCreate(HandleSerial, "handle_serial_task", 2048, (void*)this, 13, NULL);
}

void NetManager::HandleSerial(void* param)
{
    // TODO fix some bug in here causing an invalid load
    NetManager* _this = (NetManager*)param;

    Vector<Packet*>* rx_packets = nullptr;
    while (true)
    {
        // Delay at the start
        vTaskDelay(50 / portTICK_PERIOD_MS);

        // _this->serial->Rx(xTaskGetTickCount() / portTICK_PERIOD_MS);
        // printf("Afer rx\n\r");
        _this->serial->RxTx(xTaskGetTickCount() / portTICK_PERIOD_MS);

        if (!_this->serial->HasRxPackets()) continue;

        rx_packets = &_this->serial->GetRxPackets();
        uint32_t timeout = (xTaskGetTickCount() / portTICK_PERIOD_MS) + 10000;

        while (rx_packets->size() > 0 &&
            xTaskGetTickCount() / portTICK_PERIOD_MS < timeout)
        {
            printf("Net: Message from ui chip - \n\r");
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
                    printf("%d", (char)data);
                }
                else
                {
                    printf("%d", (int)data);
                }
            }
            printf("\n\r");

            if (packet_type == Packet::Types::Message)
            {
                // Pass the message to the next layer of processing...
                // aka quicr
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

void NetManager::HandleNetwork(void* param )
{
    // TODO
}

/**                          Private Functions                               **/
void NetManager::HandleSerialCommands(Packet* rx_packet)
{

    // Get the command type
    uint8_t command_type = rx_packet->GetData(24, 8);
    if (command_type == Packet::Commands::SSIDs)
    {
        // TODO
        // Get the ssids
        // WiFi.disconnect();
        // int networks = WiFi.scanNetworks();

        // if (networks == 0)
        // {
        //     Serial.println("No networks found");
        //     break;
        // }

        // Serial.print("Networks found: ");
        // Serial.println(networks);

        // Put ssids into a vector of packets and enqueue them
        // for (int i = 0; i < networks; ++i)
        // {
        //     String res = WiFi.SSID(i);

        //     if (res.length() == 0) continue;
        //     Serial.print(i + 1);
        //     Serial.print(" length - ");
        //     Serial.print(res.length());
        //     Serial.print(" - ");

        //     Packet* packet = new Packet();
        //     // Set the type
        //     packet->SetData(Packet::Types::Command, 0, 6);

        //     // Set the packet id
        //     packet->SetData(1, 6, 8);

        //     // Add 1 for the command type
        //     // Add 1 for the ssid id
        //     packet->SetData(res.length() + 2, 14, 10);

        //     // Set the first byte to the command type
        //     packet->SetData(Packet::Commands::SSIDs, 24, 8);

        //     // Set the ssid id
        //     packet->SetData(i + 1, 32, 8);
        //     for (unsigned int j = 0; j < res.length(); j++)
        //     {
        //         packet->SetData(res[j], 40 + (j * 8), 8);
        //         Serial.print((char)res[j]);
        //     }
        //     Serial.println("");
        //     ui_layer->EnqueuePacket(packet);
        // }
    }
    else if (command_type == Packet::Commands::ConnectToSSID)
    {
        // Get the ssid value, followed by the ssid_password
        unsigned char ssid_len = rx_packet->GetData(32, 8);
        printf("ssid length - %d\n\r", ssid_len);

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
        printf("SSID - ");
        printf("%s\r\n", ssid.c_str());

        unsigned char ssid_password_len = rx_packet->GetData(offset, 8);
        offset += 8;
        printf("password length - %d\n\r", ssid_password_len);

        String ssid_password;
        for (unsigned char j = 0; j < ssid_password_len; ++j)
        {
            ssid_password += static_cast<char>(rx_packet->GetData(
                offset, 8));
            offset += 8;
        }
        printf("SSID Password - %s\n\r", ssid_password.c_str());

        // TODO
        // WiFi.begin(ssid.c_str(), ssid_password.c_str());
    }
    else if (command_type == Packet::Commands::WifiStatus)
    {
        // TODO get wifi connection status
        printf("Connection status FAKE - %d\n\r", 1);

        // Create a packet that tells the current status
        Packet* connected_packet = new Packet();
        connected_packet->SetData(Packet::Types::Command, 0, 6);
        connected_packet->SetData(1, 6, 8);
        connected_packet->SetData(2, 14, 10);
        connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
        // connected_packet->SetData(WiFi.isConnected(), 32, 8);
        connected_packet->SetData(1, 32, 8);

        serial->EnqueuePacket(connected_packet);
    }
}
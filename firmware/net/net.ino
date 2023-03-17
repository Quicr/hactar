#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "inc/ModuleClient.hh"
#include "inc/SerialEsp.hh"
#include "shared_inc/SerialManager.hh"
#include "shared_inc/Packet.hh"

/** NOTE this is temporary
* This file is just for your ssid and password
* make a file called "wifi_creds.hh" with two lines of code in it
* const char* ssid = "your_ssid";
* const char* password = "your_password";
* const char* host = "your ip address";
* const unsigned int port = 12345;
*/
#include "wifi_creds.hh"

constexpr unsigned int Byte_Size = 8;

ModuleClient* client;

SerialEsp* uart;
SerialManager* ui_layer;

unsigned long current_time = 0;

void HandleIncomingNetwork()
{

    // Get the packet from the client
    Packet recv_packet = client->GetMessage();

    if (recv_packet.GetSize() == 0) return;

    // Parse the packet, and either send it to the next layer or print it
    // to a serial
    unsigned int data_idx = 0;

    // Get packet type
    unsigned char type = recv_packet.GetData(0, 6);

    unsigned char id = recv_packet.GetData(6, 8);

    // Get the packet data size
    unsigned short size = recv_packet.GetData(14, 10);

    if (type == Packet::Types::LocalDebug)
    {
        // Dump it to the serial monitor for now
        Serial.print("Network debug message ");
        for (unsigned short i = 0; i < size; ++i)
        {
            Serial.print((char)recv_packet.GetData(24 + (i * Byte_Size), Byte_Size));
        }
        Serial.println("");
    }
    else
    {
        // TODO Make a frame buffer class that has a "complete flag"
        // For now just assume its the right size..

        // Send the serial messages to the ui
        // Enqueue  the packet to the outgoing buffer
        ui_layer->EnqueuePacket(std::move(recv_packet));

        // Dump it to the serial monitor for now
        // Serial.print("Network debug message ");

        // Serial.println(type);
        // Serial.println(id);
        // Serial.println(size);
        // for (unsigned short i = 0; i < size; ++i)
        // {
        //     Serial.print((int)recv_packet.GetData(24 + (i * Byte_Size), Byte_Size));
        //     Serial.print(" ");
        // }
        // Serial.println("");
    }
}

void HandleOutgoingNetwork()
{
    // if (current_time < outgoing_network_timeout) return;
    // outgoing_network_timeout = current_time + 5000;
    client->SendMessages();
}

void HandleIncomingSerial()
{
    // Skip if no packets have come in
    if (!ui_layer->HasRxPackets()) return;

    // Get the packets from the ui layer
    Vector<Packet*>& rx_packets = ui_layer->GetRxPackets();

    // Handle incoming packets
    while (rx_packets.size() > 0)
    {
        // Get the type
        Packet& rx_packet = *rx_packets[0];
        uint8_t packet_type = rx_packet.GetData(0, 6);

        // P_type will only be message or debug by this point
        if (packet_type == Packet::Types::Message)
        {
            // Pass the message to the client
            client->EnqueuePacket(std::move(rx_packet));
        }

        rx_packets.erase(0);
    }
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    client = new ModuleClient(host, port);

    uart = new SerialEsp(Serial);
    ui_layer = new SerialManager(uart);


    Serial.println("Waiting");
    pinMode(5, OUTPUT);

    // Give time to connect to wifi
    delay(5000);
    Serial.println("Starting");
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to wifi");
    }

    current_time = millis();

    ui_layer->RxTx(current_time);
    HandleIncomingSerial();
    HandleOutgoingNetwork();
    HandleIncomingNetwork();

    yield();
}
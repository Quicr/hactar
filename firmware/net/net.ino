#include <Arduino.h>
#include <WiFi.h>
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

HardwareSerial serial_alt(Serial1);

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
        // Dump it to the serial monitor for now
        Serial.print("Message received: ");

        Serial.print(type);
        Serial.print(" ");
        Serial.print(id);
        Serial.print(" ");
        Serial.print(size);
        Serial.print(" - ");
        for (unsigned short i = 0; i < size; ++i)
        {
            Serial.print((int)recv_packet.GetData(24 + (i * Byte_Size), Byte_Size));
            Serial.print(" ");
        }
        Serial.println("");

        // Enqueue the packet for serial transmission
        ui_layer->EnqueuePacket(std::move(recv_packet));
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

        // message from stm
        Serial.println("Message from stm");
        uint16_t data_len = rx_packet.GetData(14, 10);
        unsigned char data;
        for (uint16_t i = 0; i < data_len; ++i)
        {
            data = rx_packet.GetData(24 + (i * 8), 8);
            if ((data > '0' && data < '9') || (data > 'A' && data < 'z'))
                Serial.print((char)data);
            else
                Serial.print((int)data);
        }
        Serial.println("");

        // P_type will only be message or debug by this point
        if (packet_type == Packet::Types::Message)
        {
            // Pass the message to the client
            client->EnqueuePacket(std::move(rx_packet));
        }
        else if (packet_type == Packet::Types::Command)
        {
            // Get the command type
            uint8_t command_type = rx_packet.GetData(24, 8);
            if (command_type == Packet::Commands::SSIDs)
            {
                // Get the ssids
                int networks = WiFi.scanNetworks();

                if (networks == 0)
                {
                    Serial.println("No networks found");
                    return;
                }

                Serial.print("Networks found: ");
                Serial.println(networks);

                // Put ssids into a vector of packets and enqueue them
                for (int i = 0; i < networks; ++i)
                {
                    String res = WiFi.SSID(i);

                    if (res.length() == 0) continue;
                    Serial.print(i + 1);
                    Serial.print(" length - ");
                    Serial.print(res.length());
                    Serial.print(" - ");

                    Packet packet;
                    // Set the type
                    packet.SetData(Packet::Types::Command, 0, 6);

                    // Set the packet id
                    packet.SetData(1, 6, 8);

                    // Add 1 for the command type
                    // Add 1 for the ssid id
                    packet.SetData(res.length() + 2, 14, 10);

                    // Set the first byte to the command type
                    packet.SetData(Packet::Commands::SSIDs, 24, 8);

                    // Set the ssid id
                    packet.SetData(i+1, 32, 8);
                    for (unsigned int j = 0; j < res.length(); j++)
                    {
                        packet.SetData(res[j], 40 + (j * 8), 8);
                        Serial.print((char)res[j]);
                    }
                    Serial.println("");
                    ui_layer->EnqueuePacket(std::move(packet));
                }
            }
            else if (command_type == Packet::Commands::ConnectToSSID)
            {
                // Get the ssid value, followed by the passcode
                uint8_t ssid_id = rx_packet.GetData(32, 8);
                String passcode;

                for (unsigned int i = 0; i < data_len - 1; ++i)
                {
                    passcode.concat(rx_packet.GetData(40 + (8 * i), 8));
                }
            }
        }

        rx_packets.erase(0);
    }
}

void setup()
{
    Serial.begin(115200);
    serial_alt.begin(115200, SERIAL_8N1, 17, 18);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    client = new ModuleClient(host, port);

    uart = new SerialEsp(serial_alt);
    ui_layer = new SerialManager(uart);

    Serial.println("Waiting");

    // Give time to connect to wifi
    delay(5000);

    pinMode(19, OUTPUT);
    digitalWrite(19, LOW);
    Serial.println("Starting");
}

unsigned long send_message = 0;
void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    current_time = millis();

    ui_layer->RxTx(current_time);
    HandleIncomingSerial();
    HandleOutgoingNetwork();
    HandleIncomingNetwork();

    yield();
}
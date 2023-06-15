#include <Arduino.h>
#include <WiFi.h>
#include "inc/ModuleClient.hh"
#include "inc/SerialEsp.hh"
#include "shared_inc/SerialManager.hh"
#include "shared_inc/Packet.hh"

const unsigned char Enable_Serial_Pin = 19;
const unsigned char LED_B_Pin = 5;
const unsigned char LED_G_Pin = 6;
const unsigned char LED_R_Pin = 7;


// NOTE
// These values are relative to your server that the net chip should
// connect to. 60777 is the default port for the echo_server.
const char* host = "192.168.1.120";
const unsigned int port = 60777;

constexpr unsigned int Byte_Size = 8;

ModuleClient* client;

SerialEsp* uart;
SerialManager* ui_layer;

unsigned long current_time = 0;

HardwareSerial serial_alt(Serial1);

String ssid;
String password;

void HandleIncomingNetwork()
{

    // Get the packet from the client
    Packet* recv_packet = client->GetMessage();

    if (recv_packet->GetSize() == 0)
    {
        delete recv_packet;
        return;
    }

    // Parse the packet, and either send it to the next layer or print it
    // to a serial
    // unsigned int data_idx = 0;

    // Get packet type
    unsigned char type = recv_packet->GetData(0, 6);

    unsigned char id = recv_packet->GetData(6, 8);

    // Get the packet data size
    unsigned short size = recv_packet->GetData(14, 10);

    if (type == Packet::Types::LocalDebug)
    {
        // Dump it to the serial monitor for now
        Serial.print("Network debug message ");
        for (unsigned short i = 0; i < size; ++i)
        {
            Serial.print((char)recv_packet->GetData(24 + (i * Byte_Size), Byte_Size));
        }
        Serial.println("");
        delete recv_packet;
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
            Serial.print((int)recv_packet->GetData(24 + (i * Byte_Size), Byte_Size));
            Serial.print(" ");
        }
        Serial.println("");

        // Enqueue the packet for serial transmission
        ui_layer->EnqueuePacket(recv_packet);
    }

    recv_packet = nullptr;
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

    digitalWrite(LED_R_Pin, LOW);

    // Handle incoming packets
    while (rx_packets.size() > 0)
    {
        // Get the type
        Packet* rx_packet = rx_packets[0];
        uint8_t packet_type = rx_packet->GetData(0, 6);

        // message from stm
        Serial.println("Message from stm");
        uint16_t data_len = rx_packet->GetData(14, 10);
        unsigned char data;
        for (uint16_t i = 0; i < data_len; ++i)
        {
            data = rx_packet->GetData(24 + (i * 8), 8);
            if ((data >= '0' && data <= '9') || (data >= 'A' && data <= 'z'))
            {
                Serial.print((char)data);
            }
            else
            {
                Serial.print((int)data);
                Serial.print(" ");
            }
        }
        Serial.println("");

        // P_type will only be message or debug by this point
        if (packet_type == Packet::Types::Message)
        {
            // Pass the message to the client
            client->EnqueuePacket(rx_packet);
        }
        else if (packet_type == Packet::Types::Command)
        {
            // Get the command type
            uint8_t command_type = rx_packet->GetData(24, 8);
            if (command_type == Packet::Commands::SSIDs)
            {
                // Get the ssids
                WiFi.disconnect();
                int networks = WiFi.scanNetworks();

                if (ssid.length() != 0 && password.length() != 0)
                {
                    WiFi.begin(ssid.c_str(), password.c_str());
                }

                if (networks == 0)
                {
                    Serial.println("No networks found");
                    break;
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

                    Packet* packet = new Packet();
                    // Set the type
                    packet->SetData(Packet::Types::Command, 0, 6);

                    // Set the packet id
                    packet->SetData(1, 6, 8);

                    // Add 1 for the command type
                    // Add 1 for the ssid id
                    packet->SetData(res.length() + 2, 14, 10);

                    // Set the first byte to the command type
                    packet->SetData(Packet::Commands::SSIDs, 24, 8);

                    // Set the ssid id
                    packet->SetData(i+1, 32, 8);
                    for (unsigned int j = 0; j < res.length(); j++)
                    {
                        packet->SetData(res[j], 40 + (j * 8), 8);
                        Serial.print((char)res[j]);
                    }
                    Serial.println("");
                    ui_layer->EnqueuePacket(packet);
                }
            }
            else if (command_type == Packet::Commands::ConnectToSSID)
            {
                // Get the ssid value, followed by the ssid_password
                unsigned char ssid_len = rx_packet->GetData(32, 8);
                Serial.print("ssid length - ");
                Serial.println(ssid_len);

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
                Serial.print("SSID - ");
                Serial.println(ssid);

                unsigned char ssid_password_len = rx_packet->GetData(offset, 8);
                offset += 8;
                Serial.print("password length - ");
                Serial.println(ssid_password_len);

                String ssid_password;
                for (unsigned char j = 0; j < ssid_password_len; ++j)
                {
                    ssid_password += static_cast<char>(rx_packet->GetData(
                        offset, 8));
                    offset += 8;
                }
                Serial.println("");
                Serial.print("REMOVE ME SSID Password - ");
                Serial.println(ssid_password);

                WiFi.begin(ssid.c_str(), ssid_password.c_str());
            }
            else if (command_type == Packet::Commands::WifiStatus)
            {
                Serial.print("Connection status - ");
                Serial.println(WiFi.isConnected());
                // Create a packet that tells the current status

                Packet* connected_packet = new Packet();
                connected_packet->SetData(Packet::Types::Command, 0, 6);
                connected_packet->SetData(1, 6, 8);
                connected_packet->SetData(2, 14, 10);
                connected_packet->SetData(Packet::Commands::WifiStatus, 24, 8);
                connected_packet->SetData(WiFi.isConnected(), 32, 8);

                ui_layer->EnqueuePacket(connected_packet);
            }
        }

        // Set to nullptr and delete from vector
        rx_packet = nullptr;
        rx_packets.erase(0);
    }

    digitalWrite(LED_R_Pin, HIGH);
}

void setup()
{
    Serial.begin(115200);
    serial_alt.begin(115200, SERIAL_8N1, 18, 17);
    WiFi.mode(WIFI_STA);
    // WiFi.begin(ssid, password);

    client = new ModuleClient(host, port);

    uart = new SerialEsp(serial_alt);
    ui_layer = new SerialManager(uart);

    pinMode(Enable_Serial_Pin, OUTPUT);
    digitalWrite(Enable_Serial_Pin, LOW);
    Serial.println("Done setup");

    // LED for pinging
    pinMode(LED_B_Pin, OUTPUT);
    digitalWrite(LED_B_Pin, HIGH);

    // LED for serial rx
    pinMode(LED_R_Pin, OUTPUT);
    digitalWrite(LED_R_Pin, HIGH);

    // LED for serial tx
    pinMode(LED_G_Pin, OUTPUT);
    digitalWrite(LED_G_Pin, HIGH);
}

unsigned long ping = 0;
void loop()
{
    current_time = millis();

    if (current_time > ping)
    {
        Serial.println("-- Alive --");
        ping = current_time + 10000;
        Serial.print("-- Wifi status: ");
        Serial.print(WiFi.status());
        Serial.println(" --");
    }

    ui_layer->RxTx(current_time);
    HandleIncomingSerial();
    HandleOutgoingNetwork();
    HandleIncomingNetwork();

    yield();
}

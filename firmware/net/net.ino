#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "inc/ModuleClient.hh"
#include "shared_inc/Packet.hh"

/** NOTE this is temporary
* This file is just for your ssid and password
* make a file called "wifi_creds.hh" with two lines of code in it
* const char* ssid = "your_ssid";
* const char* password = "your_password";
*/
#include "wifi_creds.hh"

constexpr unsigned int Byte_Size = 8;

Vector<Packet> outgoing_serial;

char* host = "192.168.1.120";
unsigned int port = 60777;
ModuleClient client(host, port);

// Timeouts
unsigned long current_time = 0;
unsigned long outgoing_network_timeout = 0;
unsigned long incoming_network_timeout = 0;
unsigned long outgoing_serial_timeout = 0;
unsigned long incoming_serial_timeout = 0;

void HandleIncomingNetwork()
{
    if (current_time < incoming_network_timeout) return;

    // Get the packet from the client
    Packet recv_packet = client.GetMessage();

    if (recv_packet.GetSize() == 0) return;
    incoming_network_timeout = current_time + 5000;

    // Parse the packet, and either send it to the next layer or print it
    // to a serial
    unsigned int data_idx = 0;

    // Get packet type
    unsigned char type = recv_packet.GetData(0, 6);

    unsigned char id = recv_packet.GetData(6, 8);

    // Get the packet data size
    unsigned short size = recv_packet.GetData(14, 10);

    if (type == Packet::PacketTypes::NetworkDebug)
    {
        // Dump it to the serial monitor for now
        Serial.print("Network debug message ");
        for (unsigned short i = 0; i < size; ++i)
        {
            Serial.print((char)recv_packet.GetData(16 + (i * Byte_Size), Byte_Size));
        }
        Serial.println("");
    }
    else
    {
        // TODO Make a frame buffer class that has a "complete flag"
        // For now just assume its the right size..

        // Send the serial messages to the ui
        // Enqueue  the packet to the outgoing buffer
        outgoing_serial.push_back(std::move(recv_packet));
    }
}

void HandleOutgoingNetwork()
{
    if (current_time < outgoing_network_timeout) return;
    outgoing_network_timeout = current_time + 5000;
    client.SendMessages();

}

void HandleIncomingSerial()
{
    if (current_time < incoming_serial_timeout) return;
    incoming_serial_timeout = current_time + 1000;

    // If there are no messages just skip it then
    if (!Serial.available()) return;

    Packet incoming_packet(1);
    unsigned int offset = 0;
    while (Serial.available())
    {
        incoming_packet.SetData(Serial.read(), offset, Byte_Size);
        offset += Byte_Size;
    }

    client.EnqueuePacket(incoming_packet);
}

void HandleOutgoingSerial()
{
    if (current_time < outgoing_serial_timeout) return;
    outgoing_serial_timeout = current_time + 1000;

    if (!Serial.availableForWrite()) return;

    // Print the vector and clear it for now
    // Serial.print("Outgoing serial message ");
    unsigned int size = 0;
    for (unsigned int i = 0; i < outgoing_serial.size(); ++i)
    {
        // Get the size + 3 for the first three bytes being the type
        // and data length
        size = outgoing_serial[i].GetData(14, 10) + 3;

        Serial.write(0xFF);
        for (unsigned int j = 0; j < size; ++j)
        {
            Serial.write((unsigned char)outgoing_serial[i].GetData((j * Byte_Size), Byte_Size));
        }
    }

    outgoing_serial.clear();

}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    delay(10000);
}

void loop()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Not connected to wifi");
    }

    HandleIncomingSerial();
    HandleOutgoingNetwork();
    HandleIncomingNetwork();
    HandleOutgoingSerial();

    current_time = millis();

    yield();
}
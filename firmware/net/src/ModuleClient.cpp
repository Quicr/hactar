#include "../inc/ModuleClient.hh"

ModuleClient::ModuleClient(String host, unsigned int port)
    : client(), packets(), host(host), port(port)
{
}

ModuleClient::~ModuleClient()
{
}

bool ModuleClient::SendMessages()
{
    bool messages_sent = false;
    if (!packets.size() > 0)
        return messages_sent;
    if (!Connect())
        return messages_sent;

    unsigned int j;
    unsigned char data;

    // Try to send all messages
    while (packets.size() > 0)
    {
        // If the client disconnects stop the transmission
        if (!client.connected()) return messages_sent;

        Packet& packet = packets[0];

        for (j = 0; j < packet.SizeInBytes(); j++)
        {
            data = static_cast<unsigned char>(
                packet.GetData(j*8, 8));

            client.write(data);
            messages_sent = true;
        }

        // End the message
        client.write((uint8_t)0);

        packets.erase(0);
    }

    return messages_sent;
}

void ModuleClient::EnqueuePacket(Packet&& packet)
{
    packets.push_back(std::move(packet));
}

// TODO test with Packet and Packet& because I had to add the constructor for
// Packet(const Packet& other) for return type Packet to work.
// TODO need to read response from the server saying it received a message with
// specific ID
Packet ModuleClient::GetMessage()
{
    Packet packet;
    GetMessage(packet);
    return packet;
}

bool ModuleClient::GetMessage(Packet& incoming_packet)
{
    // TODO get a message, put it on a vector
    // TODO a return messages function
    // TODO a "run module client function"
    constexpr unsigned int Byte_Size = 8;

    unsigned long current_time = millis();
    unsigned long timeout = current_time + 2000;

    if (!client.available()) return false;

    // Serial.println("Client has data to receive");
    unsigned int bit_offset = 0;
    while (client.available())
    {
        unsigned char in_byte = static_cast<unsigned char>(client.read());
        incoming_packet.SetData(in_byte, bit_offset, Byte_Size);
        bit_offset += Byte_Size;
    }
    return true;
}

void ModuleClient::SendSSIDs()
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
        Serial.print(" ");
        Serial.println(res);

        Packet packet;
        packet.SetData(Packet::Types::SSID, 0, 6);
        packet.SetData(1, 6, 8);
        packet.SetData(res.length(), 14, 10);

        for (unsigned int j = 0; j < res.length(); j++)
        {
            packet.SetData(res[j], 24 + (j * 8), 8);
        }

        EnqueuePacket(std::move(packet));
    }
}

bool ModuleClient::Connect()
{
    unsigned int attempts = 0;
    while (!client.connect(host.c_str(), port, 1000))
    {
        Serial.print("Attempting to connect to host ");
        Serial.println(attempts);
        if (++attempts > 9)
        {
            return false; // failed to connect not sending message.
        }
        delay(500);
    }
    return true;
}
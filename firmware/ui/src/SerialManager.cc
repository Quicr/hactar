#include "SerialManager.hh"

SerialManager::SerialManager(SerialInterface* uart) :
    uart(uart),
    rx_parsed_packets(10),
    rx_packet(nullptr),
    rx_packet_timeout(0),
    rx_next_id(0)
{}

SerialManager::~SerialManager()
{
    if (rx_packet) delete rx_packet;
    for (unsigned long i = 0; i < rx_parsed_packets.size(); i++)
    {
        delete rx_parsed_packets[i];
    }
}


SerialManager::SerialStatus SerialManager::WriteSerial(
    const Packet& packet,
    const unsigned long current_time)
{
    if (!uart->ReadyToWrite()) return SerialStatus::BUSY;

    // Get the buffer
    unsigned char* tx_buffer = std::move(packet.ToBytes());

    // Get the size
    unsigned short tx_buffer_sz = static_cast<unsigned short>(
        packet.GetData(14, 10)) + 3;

    // Write to serial
    uart->Write(tx_buffer, tx_buffer_sz);

    return SerialManager::SerialStatus::OK;
}

SerialManager::SerialStatus SerialManager::ReadSerial(
    const unsigned long current_time)
{
    if (rx_packet != nullptr && current_time > rx_packet_timeout)
    {
        delete rx_packet;
        rx_packet = nullptr;
        return SerialStatus::TIMEOUT;
    }

    if (!uart->AvailableBytes()) return SerialStatus::EMPTY;

    if (rx_packet == nullptr)
    {
        // Enough bytes to determine the start of a packet and
        // packet id, type, and length
        if (uart->AvailableBytes() < 4) return SerialStatus::EMPTY;

        // Read the next byte if it is the start of a packet then continue on
        // TODO enqueue an error response?
        // This is not necessarily an error.. see the THINK below
        if (uart->Read() != 0xFF) return SerialStatus::ERROR;

        // Found the start, so create a packet
        rx_packet = new Packet(current_time);

        // Timeout the packet in 5 seconds after it has started.
        rx_packet_timeout = current_time + 5000;

        // Get the next 3 bytes and put them into the packet
        rx_packet->AppendData(uart->Read(), 8);
        rx_packet->AppendData(uart->Read(), 8);
        rx_packet->AppendData(uart->Read(), 8);

        if (rx_packet->GetData(0, 6) == Packet::PacketTypes::NetworkDebug)
        {
            // THINK read the size and ignore that many bytes?
            delete rx_packet;
            rx_packet = nullptr;
            return SerialManager::SerialStatus::EMPTY;
        }
    }

    // Get the length of the incoming message
    unsigned short data_length = rx_packet->GetData(14U, 10U);

    // Read n bytes from the ring to the packet
    while (uart->AvailableBytes())
    {
        if (rx_packet->SizeInBytes() > 256U)
        {
            delete rx_packet;
            rx_packet = nullptr;
            return SerialStatus::TIMEOUT;
        }

        rx_packet->AppendData(uart->Read(), 8U);

        if (data_length+3U == (rx_packet->BitsUsed() / 8U))
        {
            rx_parsed_packets.push_back(std::move(rx_packet));
            rx_packet = nullptr;
            return SerialStatus::OK;
        }
    }

    // Not a full transmission, so we'll wait
    return SerialStatus::PARTIAL;
}

Vector<Packet*>& SerialManager::GetPackets()
{
    return rx_parsed_packets;
}
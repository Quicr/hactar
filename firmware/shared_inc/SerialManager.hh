#pragma once

#include <map>

#include "SerialInterface.hh"
#include "Vector.hh"
#include "Packet.hh"
#include "RingBuffer.hh"

#define Start_Bytes 4

class SerialManager
{
public:
    static constexpr unsigned long Packet_Timeout = 10000;
    static constexpr unsigned char Max_Retry = 3;

    typedef enum
    {
        EMPTY,
        OK,
        PARTIAL,
        BUSY,
        ERROR,
        TIMEOUT
    } SerialStatus;

    SerialManager(SerialInterface* uart) :
        uart(uart),
        rx_packets(10),
        rx_packet(nullptr),
        rx_packet_timeout(0),
        rx_status(SerialStatus::EMPTY),
        tx_packets(10),
        tx_pending_packets(),
        tx_status(SerialStatus::EMPTY)
    {}

    ~SerialManager()
    {
        if (rx_packet) delete rx_packet;
        for (unsigned long i = 0; i < rx_packets.size(); ++i)
        {
            delete rx_packets[i];
        }
    }

    void RxTx(const unsigned long current_time)
    {
        Rx(current_time);
        Tx(current_time);
    }

    void Tx(const unsigned long current_time)
    {
        // Check pending tx packets
        HandlePendingTx(current_time);

        // Send tx packet
        tx_status = WriteSerial();
    }

    void Rx(const unsigned long current_time)
    {
        rx_status = ReadSerial(current_time);
    }

    void EnqueuePacket(Packet&& packet)
    {
        tx_packets.push_back(std::move(packet));
    }

    const bool HasRxPackets() const
    {
        return rx_packets.size();
    }

    Vector<Packet*>& GetRxPackets()
    {
        return rx_packets;
    }

    const SerialStatus GetTxStatus() const
    {
        return tx_status;
    }

    const SerialStatus GetRxStatus() const
    {
        return rx_status;
    }

private:
    SerialStatus ReadSerial(const unsigned long current_time)
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

            if (data_length+3U != (rx_packet->BitsUsed() / 8U)) continue;

            unsigned char rx_type = rx_packet->GetData(0, 6);

            SerialStatus status;

            switch (rx_type)
            {
                case (unsigned char)Packet::Types::LocalDebug:
                {
                    delete rx_packet;
                    status = SerialStatus::EMPTY;
                    break;
                }
                case (unsigned char)Packet::Types::Ok:
                {
                    // Get the id
                    unsigned char ok_id = rx_packet->GetData(24, 8);
                    tx_pending_packets.erase(ok_id);

                    delete rx_packet;
                    status = SerialStatus::OK;
                    break;
                }
                case (unsigned char)Packet::Types::Error:
                {
                    unsigned char failed_id = rx_packet->GetData(24, 8);
                    Packet& failed_packet = tx_pending_packets[failed_id];
                    EnqueuePacket(std::move(failed_packet));
                    tx_pending_packets.erase(failed_id);

                    delete rx_packet;
                    status = SerialStatus::ERROR;
                    break;
                }
                case Packet::Types::Busy:
                {
                    // TODO resend our message


                    delete rx_packet;
                    status = SerialStatus::BUSY;
                    break;
                }
                default:
                {
                    // All other packets can just be assumed to be normal
                    // for now, debug messages should go elsewhere (eeprom?)

                    // Now that we got this packet, we will respond
                    Packet ok_packet;
                    ok_packet.SetData(Packet::Types::Ok, 0, 6);
                    ok_packet.SetData(1, 6, 8); // Id here doesn't matter
                    ok_packet.SetData(1, 14, 10);

                    // Get the id of the rx packet and set it to the data
                    ok_packet.SetData(rx_packet->GetData(6, 8), 24, 8);

                    // Push the ok packet
                    EnqueuePacket(std::move(ok_packet));

                    rx_packets.push_back(std::move(rx_packet));
                    status = SerialStatus::OK;
                    break;
                }
            }
            rx_packet = nullptr;
            return status;
        }

        // Not a full transmission, so we'll wait
        return SerialStatus::PARTIAL;
    }

    inline void HandlePendingTx(const unsigned long current_time)
    {
        // Check the pending packets
        if (tx_pending_packets.size() == 0) return;

        Vector<unsigned char> remove_ids;

        for (std::pair<const unsigned char, Packet>& packet_pair : tx_pending_packets)
        {
            if (packet_pair.second.GetCreatedAt() + Packet_Timeout > current_time)
                continue;

            if (packet_pair.second.GetRetries() >= Max_Retry)
            {
                remove_ids.push_back(packet_pair.first);
                continue;
            }

            // Add to the packets to remove from the map
            remove_ids.push_back(packet_pair.first);

            // Error state for now
            // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);


            // TODO later add this in
            // Update the time on the packet
            // packet_pair.second.UpdateCreatedAt(current_time);

            // Increment the retry on the packet
            // packet_pair.second.IncrementRetry();

            // The packet has expired with no response so resend it.
            // EnqueuePacket(std::move(packet_pair.second));
        }

        // Remove resent packets from the tx_pending_packets map
        for (unsigned char i = 0; i < remove_ids.size(); i++)
        {
            tx_pending_packets.erase(remove_ids[i]);
        }
    }

    SerialStatus WriteSerial()
    {
        if (tx_packets.size() == 0) return SerialStatus::EMPTY;

        if (!uart->ReadyToWrite()) return SerialStatus::BUSY;

        Packet& tx_packet = tx_packets.front();

        // Get the buffer
        unsigned char* tx_buffer = std::move(tx_packet.ToBytes());

        // Get the size
        unsigned short tx_buffer_sz = static_cast<unsigned short>(
            tx_packet.GetData(14, 10)) + 3;

        uart->Write(tx_buffer, tx_buffer_sz);

        // Check the type of packet sent
        unsigned char packet_type = tx_packet.GetData(0, 6);
        if (packet_type == Packet::Types::Message ||
            packet_type == Packet::Types::Debug)
        {
            // Get the packet id
            unsigned char packet_id = tx_packet.GetData(6, 8);

            // Move the tx packet to the sent packets
            tx_pending_packets[packet_id] = std::move(tx_packet);
        }

        // Remove the packet from tx_packets
        tx_packets.erase(0);

        return SerialStatus::OK;
    }

    SerialInterface* uart;

    // THINK should I have a map of vector of packets?
    // THINK linked list?
    // rx
    Vector<Packet*> rx_packets;
    Packet* rx_packet;
    unsigned long rx_packet_timeout;
    SerialStatus rx_status;

    // tx
    Vector<Packet> tx_packets;
    std::map<unsigned char, Packet> tx_pending_packets; // TODO max packets
    SerialStatus tx_status;

};
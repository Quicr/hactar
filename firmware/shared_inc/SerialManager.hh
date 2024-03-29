#pragma once

#include <memory>
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
        CRITICAL_ERROR,
        ERROR,
        TIMEOUT
    } SerialStatus;

    SerialManager(SerialInterface* uart) :
        uart(uart),
        rx_packets(),
        rx_packet(nullptr),
        rx_packet_timeout(0),
        rx_status(SerialStatus::EMPTY),
        tx_buffer(nullptr),
        tx_packets(),
        tx_pending_packets(),
        tx_status(SerialStatus::EMPTY),
        next_packet_id(1)
    {}

    ~SerialManager()
    {
    }

    void RxTx(const unsigned long current_time)
    {
        Rx(current_time);
        Tx(current_time);
    }

    void Rx(const unsigned long current_time)
    {
        rx_status = ReadSerial(current_time);
    }

    void Tx(const unsigned long current_time)
    {
        // Don't try to send if we are reading
        if (uart->Unread() >= 4) return;
        if (rx_packet != nullptr) return;

        // Check pending tx packets
        HandlePendingTx(current_time);

        // Send tx packet
        tx_status = WriteSerial();
    }

    void EnqueuePacket(std::unique_ptr<Packet> packet)
    {
        if (tx_packets.size() >= 10)
        {
            packet.reset();
            return;
        }

        tx_packets.push_back(std::move(packet));
    }

    bool HasRxPackets() const
    {
        return rx_packets.size();
    }

    const Vector<std::unique_ptr<Packet>>& GetRxPackets()
    {
        return rx_packets;
    }

    bool HasTxPackets() const
    {
        return tx_packets.size();
    }

    SerialStatus GetTxStatus() const
    {
        return tx_status;
    }

    SerialStatus GetRxStatus() const
    {
        return rx_status;
    }

    unsigned char NextPacketId()
    {
        if (next_packet_id == 0xFE)
            next_packet_id = 1;
        return next_packet_id++;
    }

    void LoopbackRxPacket(std::unique_ptr<Packet> packet)
    {
        rx_packets.push_back(std::move(packet));
    }

    // DEPRECATED
    void DestroyRxPacket(unsigned int idx)
    {
        // delete rx_packets[idx];
        rx_packets.erase(idx);
    }

private:
    SerialStatus ReadSerial(const unsigned long current_time)
    {
        if (rx_packet && current_time > rx_packet_timeout)
        {
            rx_packet.reset();
            return SerialStatus::TIMEOUT;
        }

        if (!uart->Unread()) return SerialStatus::EMPTY;

        if (!rx_packet)
        {
            // Enough bytes to determine the start of a packet and
            // packet id, type, and length
            if (uart->Unread() < Start_Bytes) return SerialStatus::EMPTY;

            // Read the next byte if it is the start of a packet then continue on
            // TODO enqueue an error response?
            // This is not necessarily an error.. see the THINK below
            if (uart->Read() != 0xFF)
            {
                // Error_Handler();
                return SerialStatus::CRITICAL_ERROR;
            }

            // Found the start, so create a packet
            rx_packet = std::make_unique<Packet>(current_time);

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
        while (uart->Unread())
        {
            if (rx_packet->SizeInBytes() > 256U)
            {
                rx_packet.reset();
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
                    rx_packet.reset();
                    status = SerialStatus::EMPTY;
                    break;
                }
                case (unsigned char)Packet::Types::Ok:
                {
                    // Get the id
                    unsigned char ok_id = rx_packet->GetData(24, 8);

                    // Delete the pending packet
                    tx_pending_packets[ok_id].reset();

                    // Remove the pointer from the map
                    tx_pending_packets.erase(ok_id);

                    rx_packet.reset();
                    status = SerialStatus::OK;
                    break;
                }
                case (unsigned char)Packet::Types::Error:
                {
                    unsigned char failed_id = rx_packet->GetData(24, 8);

                    EnqueuePacket(std::move(tx_pending_packets[failed_id]));
                    tx_pending_packets.erase(failed_id);

                    rx_packet.reset();
                    status = SerialStatus::ERROR;
                    break;
                }
                case (unsigned char)Packet::Types::Busy:
                {
                    // TODO resend our message


                    rx_packet.reset();
                    status = SerialStatus::BUSY;
                    break;
                }
                default:
                {
                    // All other packets can just be assumed to be normal
                    // for now, debug messages should go elsewhere (eeprom?)

                    // Now that we got this packet, we will respond
                    std::unique_ptr<Packet> ok_packet = std::make_unique<Packet>();
                    ok_packet->SetData(Packet::Types::Ok, 0, 6);
                    ok_packet->SetData(NextPacketId(), 6, 8); // Id here doesn't matter
                    ok_packet->SetData(1, 14, 10);

                    // Get the id of the rx packet and set it to the data
                    ok_packet->SetData(rx_packet->GetData(6, 8), 24, 8);

                    // Push the ok packet
                    EnqueuePacket(std::move(ok_packet));
                    ok_packet = nullptr;

                    rx_packets.push_back(std::move(rx_packet));
                    status = SerialStatus::OK;
                    break;
                }
            }
            return status;
        }

        // Not a full transmission, so we'll wait
        return SerialStatus::PARTIAL;
    }

    inline void HandlePendingTx(const unsigned long current_time)
    {
        // Check the pending packets
        if (tx_pending_packets.size() == 0) return;

        Vector<unsigned char> delete_ids;
        Vector<unsigned char> resend_ids;

        for (std::pair<const unsigned char, std::unique_ptr<Packet>>&
            packet_pair : tx_pending_packets)
        {
            if (packet_pair.second->GetCreatedAt() + Packet_Timeout > current_time)
                continue;

            if (packet_pair.second->GetRetries() >= Max_Retry)
            {
                delete_ids.push_back(packet_pair.first);
                continue;
            }

            // Add to the packets to remove from the map
            resend_ids.push_back(packet_pair.first);

            // Error state for now
            // TODO error state for the serial interface


            // TODO update the tx_pending_packets to be a pointer
            // otherwise we run into memory issues

            // Update the time on the packet
            packet_pair.second->UpdateCreatedAt(current_time);

            // Increment the retry on the packet
            packet_pair.second->IncrementRetry();

            // The packet has expired with no response so resend it.
            // EnqueuePacket(std::move(packet_pair.second));
        }

        // Remove resent packets from the tx_pending_packets map
        for (unsigned int i = 0; i < delete_ids.size(); ++i)
        {
            tx_pending_packets[delete_ids[i]].reset();
            tx_pending_packets.erase(delete_ids[i]);
        }

        for (unsigned int i = 0; i < resend_ids.size(); ++i)
        {
            EnqueuePacket(std::move(tx_pending_packets[resend_ids[i]]));

            // Remove from the pending packets
            tx_pending_packets.erase(resend_ids[i]);
        }
    }

    SerialStatus WriteSerial()
    {
        if (tx_packets.size() == 0) return SerialStatus::EMPTY;

        // Wait until the last transmission was sent
        if (!uart->ReadyToWrite()) return SerialStatus::BUSY;

        // If tx buffer is allocated delete it
        if (tx_buffer != nullptr) delete tx_buffer;

        std::unique_ptr<Packet> tx_packet = std::move(tx_packets.front());

        // Get the buffer, with a start byte of 0xFF
        tx_buffer = tx_packet->ToBytes();

        // Get the size
        unsigned short tx_buffer_sz = static_cast<unsigned short>(
            tx_packet->GetData(14, 10)) + 3;

        uart->Write(tx_buffer, tx_buffer_sz);

        // Check the type of packet sent
        unsigned char packet_type = tx_packet->GetData(0, 6);

        if (packet_type != Packet::Types::Ok ||
            packet_type != Packet::Types::Error ||
            packet_type != Packet::Types::Busy ||
            packet_type != Packet::Types::LocalDebug)
        {
            // Get the packet id
            // unsigned char packet_id = tx_packet->GetData(6, 8);

            // Move the tx packet to the sent packets
            // tx_pending_packets[packet_id] = tx_packet;
            tx_packet.reset();
        }
        else
        {
            // Otherwise just delete the packet
            tx_packet.reset();
        }

        // Remove the packet from tx_packets
        tx_packets.erase(0);

        return SerialStatus::OK;
    }

    SerialInterface* uart;

    // THINK should I have a map of vector of packets?
    // THINK linked list?
    // rx
    Vector<std::unique_ptr<Packet>> rx_packets;
    std::unique_ptr<Packet> rx_packet;
    unsigned long rx_packet_timeout;
    SerialStatus rx_status;

    // tx
    unsigned char* tx_buffer;
    Vector<std::unique_ptr<Packet>> tx_packets;
    std::map<unsigned char, std::unique_ptr<Packet>> tx_pending_packets; // TODO max packets
    SerialStatus tx_status;

    unsigned char next_packet_id;

};
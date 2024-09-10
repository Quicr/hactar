#pragma once

#include "serial_interface.hh"
#include "serial_packet.hh"
#include "ring_buffer.hh"
#include "logger.hh"

#include <deque>
#include <map>
#include <memory>
#include <vector>

#define Front_Bytes 6U
#define Start_Bytes Front_Bytes - 1U

// TODO delete old stale packets

class SerialPacketManager
{
public:
    typedef unsigned char byte_t;
    static constexpr unsigned long Packet_Timeout = 10000;
    static constexpr byte_t Max_Retry = 3;

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

    SerialPacketManager(SerialInterface* uart):
        uart(uart),
        rx_packets(),
        q_message_packets(),
        command_packets(),
        setting_packets(),
        rx_packet(nullptr),
        rx_packet_timeout(0),
        rx_status(SerialStatus::EMPTY),
        tx_buffer(nullptr),
        tx_packets(),
        tx_status(SerialStatus::EMPTY),
        next_packet_id(1)
    {
    }

    ~SerialPacketManager()
    {
    }

    void Update(const unsigned long current_time)
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
        // Send tx packet
        tx_status = WriteSerial();
    }

    void EnqueuePacket(std::unique_ptr<SerialPacket> packet)
    {
        if (tx_packets.size() >= 0xFF)
        {
            return;
        }

        tx_packets.push_back(std::move(packet));
    }

    bool HasRxPackets() const
    {
        return rx_packets.size();
    }

    std::deque<std::unique_ptr<SerialPacket>>& GetRxPackets()
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

    unsigned short NextPacketId()
    {
        if (next_packet_id == 0xFFFE)
            next_packet_id = 1;
        return next_packet_id++;
    }

    void LoopbackRxPacket(std::unique_ptr<SerialPacket> packet)
    {
        rx_packets.push_back(std::move(packet));
    }

    bool GetQMessagePackets(
        RingBuffer<std::unique_ptr<SerialPacket>>** buff,
        const SerialPacket::QMessages q_message_type) const
    {
        if (q_message_packets.find(q_message_type) ==
            q_message_packets.end())
        {
            return false;
        }

        *buff = const_cast<RingBuffer<std::unique_ptr<SerialPacket>> *>(
            &q_message_packets.at(q_message_type));

        return true;
    }

    bool GetCommandPackets(
        RingBuffer<std::unique_ptr<SerialPacket>>** buff,
        const SerialPacket::Commands command_type) const
    {
        if (command_packets.find(command_type) ==
            command_packets.end())
        {
            return false;
        }

        *buff = const_cast<RingBuffer<std::unique_ptr<SerialPacket>> *>(
            &command_packets.at(command_type));

        return true;
    }

    bool GetSettingsPackets(
        RingBuffer<std::unique_ptr<SerialPacket>>** buff,
        const SerialPacket::Settings setting_type) const
    {
        if (setting_packets.find(setting_type) ==
            setting_packets.end())
        {
            return false;
        }

        *buff = const_cast<RingBuffer<std::unique_ptr<SerialPacket>> *>(
            &setting_packets.at(setting_type));

        return true;
    }

private:
    SerialStatus ReadSerial(const unsigned long current_time)
    {
        if (rx_packet && current_time > rx_packet_timeout)
        {
            rx_packet.reset();
            return SerialStatus::TIMEOUT;
        }

        if (!uart->Unread())
        {
            return SerialStatus::EMPTY;
        }

        if (!rx_packet)
        {
            // Enough bytes to determine the start of a packet and
            // packet id, type, and length
            if (uart->Unread() < Front_Bytes)
            {
                return SerialStatus::EMPTY;
            }

            // Read the next byte if it is the start of a packet then continue on
            // TODO enqueue an error response?
            // This is not necessarily an error.. see the THINK below
            if (uart->Read() != uint16_t(0xFFU))
            {
                // Error_Handler();
                return SerialStatus::CRITICAL_ERROR;
            }

            // Type
            uint8_t type = uart->Read();

            uint16_t id = uint16_t(uart->Read()) | (uint16_t(uart->Read()) << 8);
            uint16_t length = uint16_t(uart->Read()) | (uint16_t(uart->Read()) << 8);

            // Timeout the packet in 5 seconds after it has started.
            rx_packet_timeout = current_time + 5000;

            // Found the start, so create a packet
            rx_packet = std::make_unique<SerialPacket>(
                current_time,
                length + Start_Bytes
            );

            rx_packet->SetData(type, 0, 1);

            // Id
            rx_packet->SetData(id, 1, 2);

            // Length
            rx_packet->SetData(length, 3, 2);
        }

        SerialStatus status = SerialStatus::PARTIAL;

        // Get the length of the incoming message
        unsigned short data_length = rx_packet->GetData<unsigned short>(3, 2);

        // Read n bytes from the ring to the packet
        while (uart->Unread())
        {
            if (rx_packet->NumBytes() > 0xFFFF)
            {
                rx_packet.reset();
                return SerialStatus::TIMEOUT;
            }
            rx_packet->SetData(uart->Read(), 1);

            // If we haven't received all of the bytes yet then keep looping
            // Through the data
            if (data_length + Start_Bytes != rx_packet->NumBytes())
            {
                continue;
            }

            auto rx_type =
                (SerialPacket::Types)rx_packet->GetData<byte_t>(0, 1);

            switch (rx_type)
            {
                case SerialPacket::Types::LocalDebug:
                {
                    rx_packet.reset();
                    status = SerialStatus::EMPTY;
                    break;
                }
                case SerialPacket::Types::Command:
                {
                    auto command_type = static_cast<SerialPacket::Commands>(
                        rx_packet->GetData<unsigned short>(5, 2));

                    command_packets[command_type].Write(std::move(rx_packet));
                    status = SerialStatus::OK;
                    break;
                }
                // TODO messages
                default:
                {
                    // All other packets can just be assumed to be normal
                    // for now, debug messages should go elsewhere (eeprom?)

                    rx_packets.push_back(std::move(rx_packet));
                    status = SerialStatus::OK;
                    break;
                }
            }

            Logger::Log(Logger::Level::Info, "Received serial packet");
            break;
        }

        // Not a full transmission, so we'll wait
        return status;
    }

    SerialStatus WriteSerial()
    {
        // Wait until the last transmission was sent
        if (!uart->TransmitReady())
        {
            return SerialStatus::BUSY;
        }

        if (tx_buffer != nullptr)
        {
            tx_buffer = nullptr;
            tx_packet.reset();
        }

        if (tx_packets.size() == 0)
        {
            return SerialStatus::EMPTY;
        }

        tx_packet = std::move(tx_packets.front());

        tx_buffer = tx_packet->Data();

        // Get the size
        uint16_t tx_buffer_sz =
            tx_packet->GetData<unsigned short>(3, 2) + Start_Bytes;

        uart->Transmit(tx_buffer, tx_buffer_sz);
        tx_packets.pop_front();
        return SerialStatus::OK;
    }

    SerialInterface* uart;

    // rx
    std::deque<std::unique_ptr<SerialPacket>> rx_packets;

    // TODO packet bucket class?
    std::map<SerialPacket::QMessages,
        RingBuffer<std::unique_ptr<SerialPacket>>> q_message_packets;
    std::map<SerialPacket::Commands,
        RingBuffer<std::unique_ptr<SerialPacket>>> command_packets;
    std::map<SerialPacket::Settings,
        RingBuffer<std::unique_ptr<SerialPacket>>> setting_packets;

    std::unique_ptr<SerialPacket> rx_packet;
    unsigned long rx_packet_timeout;
    SerialStatus rx_status;

    // tx
    byte_t* tx_buffer;
    std::unique_ptr<SerialPacket> tx_packet;
    std::deque<std::unique_ptr<SerialPacket>> tx_packets;
    SerialStatus tx_status;

    uint16_t next_packet_id;

};
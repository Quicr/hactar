#include "serial_handler.hh"
#include "logger.hh"
#include <algorithm>
#include <cstring>

SerialHandler::SerialHandler(const uint16_t num_rx_packets,
                             uint8_t& tx_buff,
                             const uint32_t tx_buff_sz,
                             uint8_t& rx_buff,
                             const uint32_t rx_buff_sz,
                             void (*Transmit)(void* arg),
                             void* transmit_arg) :
    rx_packets(num_rx_packets),
    tx_buff(&tx_buff),
    tx_buff_sz(tx_buff_sz),
    rx_buff(&rx_buff),
    rx_buff_sz(rx_buff_sz),
    tx_write_idx(0),
    tx_read_idx(0),
    tx_free(true),
    unsent(0),
    num_to_send(0),
    rx_write_idx(0),
    rx_read_idx(0),
    unread(0),
    update_cache(0),
    Transmit(Transmit),
    transmit_arg(transmit_arg),
    packet(&rx_packets.Write()),
    bytes_read(0),
    sync_matched(0),
    escaped(false)
{
}

SerialHandler::~SerialHandler()
{
    Transmit = nullptr;
    transmit_arg = nullptr;
}

// TODO optimize our buffers

link_packet_t* SerialHandler::Read()
{
    return TLVRead();
}

void SerialHandler::Write(const uint8_t data, const bool end_frame)
{
    Write(&data, 1, end_frame);
}

void SerialHandler::Write(const link_packet_t& packet, const bool end_frame)
{
    Write(packet.PacketData(), end_frame);
}

void SerialHandler::Write(std::span<const uint8_t> data, const bool end_frame)
{
    Write(data.data(), data.size(), end_frame);
}

void SerialHandler::Write(const uint8_t* data, const uint16_t size, const bool end_frame)
{
#ifdef PLATFORM_ESP
    std::lock_guard<std::mutex> _(write_mux);
#endif

    TLVWrite(data, size);

    if (unsent > tx_buff_sz)
    {
        // TODO ERROR
        Logger::Log(Logger::Level::Error, "Transmit buffer full");
    }

    if (!tx_free)
    {
        return;
    }

    // Logger::Log(Logger::Level::Info, "Transmit");
    PrepTransmit();
}

uint16_t SerialHandler::Unread()
{
    return unread;
}

uint16_t SerialHandler::Unsent()
{
    return unsent;
}

void SerialHandler::WriteToTxBuff(const uint8_t data)
{
    if (tx_write_idx >= tx_buff_sz)
    {
        tx_write_idx = 0;
    }
    tx_buff[tx_write_idx++] = data;
}

uint8_t SerialHandler::ReadFromRxBuff()
{
    if (rx_read_idx >= rx_buff_sz)
    {
        rx_read_idx = 0;
    }
    return rx_buff[rx_read_idx++];
}

void SerialHandler::UpdateRx(const uint16_t num_recv)
{
    unread += num_recv;
    rx_write_idx += num_recv;
    if (rx_write_idx >= rx_buff_sz)
    {
        rx_write_idx = 0;
    }

    if (unread > rx_buff_sz)
    {
        // TODO
        // Error("SerialStm rx isr", "Overflowed rx buffer");
    }
}

bool SerialHandler::UpdateTx()
{
    // TODO esp semaphore of send
    unsent -= num_to_send;
    tx_read_idx += num_to_send;

    if (tx_read_idx >= tx_buff_sz)
    {
        tx_read_idx = 0;
    }

    return PrepTransmit();
}

bool SerialHandler::PrepTransmit()
{
    // Nothing to send
    if (unsent == 0)
    {
        tx_free = true;
        return false;
    }
    tx_free = false;
    num_to_send = unsent;

    // Prevent tx buff overflow
    if (num_to_send + tx_read_idx >= tx_buff_sz)
    {
        num_to_send = tx_buff_sz - tx_read_idx;
    }

    Transmit(transmit_arg);
    return true;
}

link_packet_t* SerialHandler::TLVRead()
{
    uint16_t total_bytes_read = 0;
    auto packet_data = packet->WriteableData();

    while (total_bytes_read + update_cache < unread)
    {
        uint8_t byte = ReadFromRxBuff();
        ++total_bytes_read;

        if (sync_matched < link_packet_t::Sync_Word_Size)
        {
            if (byte == packet->sync_word[sync_matched])
            {
                ++sync_matched;
            }
            else
            {
                sync_matched = byte == packet->sync_word[0];
                packet->is_ready = false;

                Logger::Log(Logger::Level::Info, "TLV error in sync word");
            }

            continue;
        }

        packet_data[bytes_read++] = byte;
        if (bytes_read < link_packet_t::Header_Size)
        {
            continue;
        }

        if (bytes_read >= packet->length + link_packet_t::Header_Size)
        {
            bytes_read = 0;
            sync_matched = 0;

            packet->is_ready = true;
            packet = &rx_packets.Write();
            packet_data = packet->WriteableData();
            continue;
        }
        else if (bytes_read >= link_packet_t::Packet_Size)
        {
            Logger::Log(Logger::Level::Info, "TLV frame size error");

            packet->is_ready = false;
            bytes_read = 0;
            sync_matched = 0;
            continue;
        }
    }

    if (total_bytes_read > 0 || update_cache > 0)
    {
        UpdateUnread(total_bytes_read);
    }

    return GetReadyPacket();
}

link_packet_t* SerialHandler::GetReadyPacket()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* p = &rx_packets.Read();
    p->is_ready = false;
    return p;
}

void SerialHandler::TLVWrite(const uint8_t* data, const uint16_t size)
{
    for (uint16_t i = 0; i < size; ++i)
    {
        WriteToTxBuff(data[i]);
    }

    unsent += size;
}

void SerialHandler::ReplyAck()
{
    link_packet_t packet;
    packet.type = Response_Ack;
    packet.length = 0;
    Write(packet);
}

void SerialHandler::ReplyError()
{
    link_packet_t packet;
    packet.type = Response_Error;
    packet.length = 0;
    Write(packet);
}

void SerialHandler::Reply(uint16_t type)
{
    link_packet_t packet;
    packet.type = type;
    packet.length = 0;
    Write(packet);
}

void SerialHandler::Reply(uint16_t type, const std::string& data)
{
    Reply(type,
          std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

void SerialHandler::Reply(uint16_t type, std::span<const uint8_t> data)
{
    link_packet_t packet;
    packet.type = type;
    packet.length = data.size();
    if (!data.empty())
    {
        std::memcpy(packet.payload.data(), data.data(),
                    std::min(data.size(), packet.payload.size()));
    }
    Write(packet);
}

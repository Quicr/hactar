#include "uart_handler.hh"
#include "logger.hh"
#include <memory.h>

UartHandler::UartHandler(const uint16_t num_rx_packets,
                         uint8_t& tx_buff,
                         const uint32_t tx_buff_sz,
                         uint8_t& rx_buff,
                         const uint32_t rx_buff_sz,
                         void (*Transmit)(void* arg),
                         void* transmit_arg,
                         const bool use_slip) :
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
    use_slip(use_slip),
    packet(&rx_packets.Write()),
    bytes_read(0),
    escaped(false)
{
}

UartHandler::~UartHandler()
{
    Transmit = nullptr;
    transmit_arg = nullptr;
}

// TODO optimize our buffers

link_packet_t* UartHandler::Read()
{
    if (use_slip)
    {
        return SlipRead();
    }
    else
    {
        return TLVRead();
    }
}

void UartHandler::Write(const uint8_t data, const bool end_frame)
{
    Write(&data, 1, end_frame);
}

void UartHandler::Write(const link_packet_t& packet, const bool end_frame)
{
    Write(packet.data, packet.length + link_packet_t::Header_Size, end_frame);
}

void UartHandler::Write(const uint8_t* data, const uint16_t size, const bool end_frame)
{
#ifdef PLATFORM_ESP
    std::lock_guard<std::mutex> _(write_mux);
#endif

    if (use_slip)
    {
        SlipWrite(data, size, end_frame);
    }
    else
    {
        TLVWrite(data, size);
    }

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

uint16_t UartHandler::Unread()
{
    return unread;
}

uint16_t UartHandler::Unsent()
{
    return unsent;
}

void UartHandler::WriteToTxBuff(const uint8_t data)
{
    if (tx_write_idx >= tx_buff_sz)
    {
        tx_write_idx = 0;
    }
    tx_buff[tx_write_idx++] = data;
}

uint8_t UartHandler::ReadFromRxBuff()
{
    if (rx_read_idx >= rx_buff_sz)
    {
        rx_read_idx = 0;
    }
    return rx_buff[rx_read_idx++];
}

void UartHandler::UpdateRx(const uint16_t num_recv)
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

bool UartHandler::UpdateTx()
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

bool UartHandler::PrepTransmit()
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

link_packet_t* UartHandler::SlipRead()
{
    // TODO for some reason when the semaphore can't be taken
    // we get a lot of frame errors
    // even though our update cache should be handling that
    // by offsetting the total bytes read
    uint16_t total_bytes_read = 0;
    uint8_t byte = 0;
    while (total_bytes_read + update_cache < unread)
    {
        // Get a byte
        byte = ReadFromRxBuff();
        ++total_bytes_read;

        // Note if we don't cast everything, esp32 freaks out
        if (byte == END
            && uint16_t(bytes_read) == uint16_t(packet->length + link_packet_t::Header_Size))
        {
            packet->is_ready = true;

            // Null out our packet pointer
            packet = &rx_packets.Write();
            bytes_read = 0;
            continue;
        }
        else if (byte == END)
        {
            Logger::Log(Logger::Level::Info, "Frame len error");

            packet->is_ready = false;
            bytes_read = 0;
            continue;
        }

        if (bytes_read >= link_packet_t::Packet_Size)
        {
            Logger::Log(Logger::Level::Info, "Frame overflow error");
            // Hit maximum size and didn't get an end packet

            packet->is_ready = false;
            bytes_read = 0;
            continue;
        }

        if (byte == ESC)
        {
            escaped = true;
            continue;
        }

        if (escaped)
        {
            if (byte == ESC_END)
            {
                packet->data[bytes_read++] = END;
            }
            else if (byte == ESC_ESC)
            {
                packet->data[bytes_read++] = ESC;
            }

            escaped = false;
        }
        else
        {
            packet->data[bytes_read++] = byte;
        }
    }

    if (total_bytes_read > 0 || update_cache > 0)
    {
        UpdateUnread(total_bytes_read);
    }

    return GetReadyPacket();
}

link_packet_t* UartHandler::TLVRead()
{
    uint16_t total_bytes_read = 0;
    uint8_t byte = 0;

    while (total_bytes_read + update_cache < unread)
    {
        uint8_t byte = ReadFromRxBuff();
        packet->data[bytes_read++] = byte;
        ++total_bytes_read;

        if (bytes_read < link_packet_t::Header_Size)
        {
            continue;
        }

        if (bytes_read >= packet->length + link_packet_t::Header_Size)
        {
            bytes_read = 0;
            packet->is_ready = true;
            packet = &rx_packets.Write();
            continue;
        }
        else if (bytes_read >= link_packet_t::Packet_Size)
        {
            Logger::Log(Logger::Level::Info, "TLV frame size error");

            packet->is_ready = false;
            bytes_read = 0;
            continue;
        }
    }

    if (total_bytes_read > 0 || update_cache > 0)
    {
        UpdateUnread(total_bytes_read);
    }

    return GetReadyPacket();
}

link_packet_t* UartHandler::GetReadyPacket()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* p = &rx_packets.Read();
    p->is_ready = false;
    return p;
}

void UartHandler::SlipWrite(const uint8_t* data, const uint16_t size, const bool end_frame)
{
    for (uint16_t i = 0; i < size; ++i)
    {
        if (data[i] == END)
        {
            unsent += 1;
            WriteToTxBuff(ESC);
            WriteToTxBuff(ESC_END);
        }
        else if (data[i] == ESC)
        {
            unsent += 1;
            WriteToTxBuff(ESC);
            WriteToTxBuff(ESC_ESC);
        }
        else
        {
            WriteToTxBuff(data[i]);
        }
    }

    unsent += size;
    // End frame
    if (end_frame)
    {
        WriteToTxBuff(END);
        unsent += 1;
    }
}

void UartHandler::TLVWrite(const uint8_t* data, const uint16_t size)
{
    for (uint16_t i = 0; i < size; ++i)
    {
        WriteToTxBuff(data[i]);
    }

    unsent += size;
}

void UartHandler::ReplyAck()
{
    Write(const_cast<uint8_t*>(&Ack), 1);
}

void UartHandler::ReplyNack()
{
    Write(const_cast<uint8_t*>(&Nack), 1);
}
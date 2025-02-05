#include "serial_handler.hh"

#include "../../shared_inc/packet_builder.hh"

#include <memory.h>

SerialHandler::SerialHandler(const uint16_t num_rx_packets,
    uint8_t& tx_buff, const uint32_t tx_buff_sz,
    uint8_t& rx_buff, const uint32_t rx_buff_sz,
    void (*Transmit)(void* arg),
    void* transmit_arg):
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
    Transmit(Transmit),
    transmit_arg(transmit_arg)
{

}

SerialHandler::~SerialHandler()
{
    Transmit = nullptr;
    transmit_arg = nullptr;
}


link_packet_t* SerialHandler::Read()
{
    while (unread > 0)
    {
        uint32_t num_bytes = unread;
        if (rx_read_idx + num_bytes > rx_buff_sz)
        {
            num_bytes = rx_buff_sz - rx_read_idx;
        }

        BuildPacket(rx_buff + rx_read_idx, num_bytes, rx_packets);

        unread -= num_bytes;
        rx_read_idx += num_bytes;

        if (rx_read_idx >= rx_buff_sz)
        {
            rx_read_idx = 0;
        }
    }

    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* p = &rx_packets.Read();
    p->is_ready = false;
    return p;
}

void SerialHandler::Write(const uint8_t data)
{
    Write(&data, 1);
}

void SerialHandler::Write(const link_packet_t& packet)
{
    Write(packet.data, packet.length + link_packet_t::Header_Size);
}

void SerialHandler::Write(const uint8_t* data, const uint16_t size)
{
    uint16_t total_bytes = size;
    uint16_t offset = 0;

    // Check if overflow
    if (unsent + total_bytes > tx_buff_sz)
    {
        // TODO
        // Error("SerialHandler write", "Transmit buffer overflow!");
    }

    if (tx_write_idx + total_bytes >= tx_buff_sz)
    {
        uint16_t diff = tx_buff_sz - tx_write_idx;

        // copy into tx_buff
        memcpy(tx_buff + tx_write_idx, data, diff);

        total_bytes -= diff;
        offset += diff;
        tx_write_idx = 0;
    }

    memcpy(tx_buff + tx_write_idx, data + offset, total_bytes);

    tx_write_idx += total_bytes;
    unsent += size;

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

void SerialHandler::PrepTransmit()
{
    // Nothing to send
    if (unsent == 0)
    {
        tx_free = true;
        return;
    }
    tx_free = false;
    num_to_send = unsent;

    // Prevent tx buff overflow
    if (num_to_send + tx_read_idx >= tx_buff_sz)
    {
        num_to_send = tx_buff_sz - tx_read_idx;
    }

    Transmit(transmit_arg);
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

void SerialHandler::UpdateTx()
{
    unsent -= num_to_send;
    tx_read_idx += num_to_send;

    if (tx_read_idx >= tx_buff_sz)
    {
        tx_read_idx = 0;
    }

    PrepTransmit();
}
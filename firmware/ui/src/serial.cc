#include "serial.hh"
#include "packet_builder.hh"

#include <memory.h>


// TODO unify into ui and net

Serial::Serial(UART_HandleTypeDef* uart, const uint16_t num_rx_packets,
    uint8_t& tx_buff, const uint32_t tx_buff_sz,
    uint8_t& rx_buff, const uint32_t rx_buff_sz):
    uart(uart),
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
    unread(0)
{

}

Serial::~Serial()
{
    uart = nullptr;
}

void Serial::StartReceive()
{
    HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_buff, rx_buff_sz);
}

void Serial::Reset()
{
    HAL_UART_AbortReceive_IT(uart);
    uint16_t err = uart->Instance->SR;
    UNUSED(err);
    StartReceive();
}

link_packet_t* Serial::Read()
{
    while (unread > 0)
    {
        uint16_t bytes_to_read = unread;
        if (rx_read_idx + bytes_to_read > rx_buff_sz)
        {
            bytes_to_read = rx_buff_sz - rx_read_idx;
        }

        // Builds packet and sets is_ready to true when a packet is done.
        BuildPacket(rx_buff + rx_read_idx, bytes_to_read, rx_packets);
        unread -= bytes_to_read;
        rx_read_idx += bytes_to_read;

        if (rx_read_idx >= rx_buff_sz)
        {
            rx_read_idx = 0;
        }
    }

    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* packet = &rx_packets.Read();
    packet->is_ready = false;
    return packet;
}

void Serial::Write(const uint8_t data)
{
    Write(&data, 1);
}

void Serial::Write(const link_packet_t& packet)
{
    Write(packet.data, packet.length + link_packet_t::Header_Size);
}

void Serial::Write(const uint8_t* data, const uint16_t size)
{
    uint16_t total_bytes = size;
    uint16_t offset = 0;

    // Check if overflow
    if (unsent + total_bytes > tx_buff_sz)
    {
        Error_Handler();
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

    Transmit(this);
}

void Serial::Transmit(Serial* self)
{
    // Nothing to send
    if (self->unsent == 0)
    {
        self->tx_free = true;
        return;
    }
    self->tx_free = false;
    self->num_to_send = self->unsent;

    // Prevent tx buff overflow
    if (self->num_to_send + self->tx_read_idx >= self->tx_buff_sz)
    {
        self->num_to_send = self->tx_buff_sz - self->tx_read_idx;
    }

    HAL_UART_Transmit_DMA(self->uart, self->tx_buff + self->tx_read_idx, self->num_to_send);
}

const UART_HandleTypeDef* Serial::UART()
{
    return uart;
}

void Serial::RxISR(Serial* self, const uint16_t fifo_idx)
{
    // NOTE- Do NOT put a breakpoint in here, the callbacks will
    // get blocked will get memory access errors because the
    // callback on your breakpoint will throw off the values and then
    // will cause the value to be at the wrong idx and everything will go
    // ka-boom, crash, plop. Overflow errors and stuff.

    const uint16_t num_recv = fifo_idx - self->rx_write_idx;
    self->rx_write_idx += num_recv;
    self->unread += num_recv;
    if (self->rx_write_idx >= self->rx_buff_sz)
    {
        self->rx_write_idx = self->rx_write_idx - self->rx_buff_sz;
    }
}

void Serial::TxISR(Serial* self)
{
    self->unsent -= self->num_to_send;
    self->tx_read_idx += self->num_to_send;

    if (self->tx_read_idx >= self->tx_buff_sz)
    {
        self->tx_read_idx = 0;
    }

    Serial::Transmit(self);
}
#include "serial.hh"
#include "packet_builder.hh"

#include <memory.h>


// TODO unify into ui and net

Serial::Serial(UART_HandleTypeDef* uart):
    uart(uart),
    tx_is_free(true),
    packet_started(false),
    rx_buff{ 0 },
    rx_packets(7),
    rx_packet(nullptr)
{

}

Serial::~Serial()
{
    uart = nullptr;
}

void Serial::StartReceive()
{
    HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_buff, Rx_Buff_Sz);
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
        if (read_idx + bytes_to_read > Rx_Buff_Sz)
        {
            bytes_to_read = Rx_Buff_Sz - read_idx;
        }

        // Builds packet and sets is_ready to true when a packet is done.
        BuildPacket(rx_buff+read_idx, bytes_to_read, rx_packets);
        unread -= bytes_to_read;
        read_idx += bytes_to_read;

        if (read_idx >= Rx_Buff_Sz)
        {
            read_idx = 0;
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
    ChangeFreeState();
    HAL_UART_Transmit_DMA(uart, &data, 1);
}

void Serial::Write(const uint8_t* data, const size_t size)
{
    ChangeFreeState();
    HAL_UART_Transmit_DMA(uart, data, size);
}

void Serial::Write(const link_packet_t& packet)
{
    ChangeFreeState();
    HAL_UART_Transmit_DMA(uart, packet.data, packet.length+Packet_Header_Size);
}

void Serial::RxEvent(const uint16_t fifo_idx)
{
    // NOTE- Do NOT put a breakpoint in here, the callbacks will
    // get blocked will get memory access errors because the
    // callback on your breakpoint will throw off the values and then
    // will cause the value to be at the wrong idx and everything will go
    // ka-boom, crash, plop. Overflow errors and stuff.

    const uint16_t num_recv = fifo_idx - write_idx;
    write_idx += num_recv;
    unread += num_recv;
    if (write_idx >= Rx_Buff_Sz)
    {
        write_idx = write_idx - Rx_Buff_Sz;
    }
}

void Serial::TxEvent()
{

}

bool Serial::IsFree()
{
    return tx_is_free;
}

void Serial::Free()
{
    tx_is_free = true;
}

void Serial::ChangeFreeState()
{
    // TODO use tx free?
    if (uart->gState != HAL_UART_STATE_READY)
    {
        // Error_Handler();
        return;
    }

    tx_is_free = false;
}
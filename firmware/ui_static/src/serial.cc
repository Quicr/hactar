#include "serial.hh"
#include "packet_builder.hh"

#include <memory.h>


// TODO unify into ui and net

Serial::Serial(UART_HandleTypeDef* uart):
    uart(uart),
    tx_is_free(true),
    packet_started(false),
    bytes_read(0),
    rx_buff{ 0 },
    rx_packets(3),
    rx_packet(nullptr),
    write_idx(0),
    read_idx(0),
    unread(0)
{

}

Serial::~Serial()
{
    uart = nullptr;
}

void Serial::StartReceive()
{
    write_idx = 0;
    read_idx = 0;
    unread = 0;
    HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_buff, Rx_Buff_Sz);
}

void Serial::Reset()
{
    HAL_UART_AbortReceive_IT(uart);
    uint16_t err = uart->Instance->SR;
    StartReceive();
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

void Serial::Write(const packet_t& packet)
{
    ChangeFreeState();
    HAL_UART_Transmit_DMA(uart, packet.data, packet.length);
}

size_t Serial::Unread()
{
    return unread;
}

void Serial::RxEvent(const uint16_t rx_idx)
{
    // NOTE- Do NOT put a breakpoint in here, the callbacks will
    // get blocked will get memory access errors because the
    // callback on your breakpoint will throw off the values and then
    // will cause the value to be at the wrong idx and everything will go
    // ka-boom, crash, plop. Overflow errors and stuff.
    const uint16_t num_recv = rx_idx - write_idx;
    write_idx += num_recv;
    if (write_idx >= Rx_Buff_Sz)
    {
        write_idx = write_idx - Rx_Buff_Sz;
    }

    if (num_recv <= 0)
    {
        return;
    }

    // Builds packet and sets is_ready to true when a packet is done.
    BuildPacket(rx_buff, num_recv, rx_packets);
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
        Error_Handler();
        return;
    }

    tx_is_free = false;
}
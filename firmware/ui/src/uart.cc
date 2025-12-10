#include "logger.hh"
#include "uart.hh"
#include <memory.h>

// TODO unify into ui and net

Uart::Uart(UART_HandleTypeDef* uart,
           const uint16_t num_rx_packets,
           uint8_t& tx_buff,
           const uint32_t tx_buff_sz,
           uint8_t& rx_buff,
           const uint32_t rx_buff_sz,
           const bool use_slip) :
    UartHandler(num_rx_packets, tx_buff, tx_buff_sz, rx_buff, rx_buff_sz, Transmit, this, use_slip),
    uart(uart)
{
}

Uart::~Uart()
{
    uart = nullptr;
}

void Uart::StartReceive()
{
    HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_buff, rx_buff_sz);
}

void Uart::Stop()
{
    HAL_UART_AbortReceive_IT(uart);
}

void Uart::Reset()
{
    rx_write_idx = 0;
    rx_read_idx = 0;

    link_packet_t* packets = rx_packets.Buffer();
    for (int i = 0; i < rx_packets.Size(); ++i)
    {
        packets[i].is_ready = false;
    }

    packet = nullptr;
    bytes_read = 0;

    StartReceive();
}

void Uart::ResetRecv()
{
    uint16_t err = uart->Instance->SR;
    Stop();
    UNUSED(err);

    Reset();
}

void Uart::UpdateUnread(const uint16_t update)
{
    // UI_LOG_INFO("unread %d, update %d", (int)unread, (int)update);
    __disable_irq();
    unread -= update;
    __enable_irq();
    // UI_LOG_INFO("unread %d", (int)unread);
}

void Uart::Transmit(void* arg)
{
    Uart* self = static_cast<Uart*>(arg);
    HAL_UART_Transmit_DMA(self->uart, self->tx_buff + self->tx_read_idx, self->num_to_send);
}

const UART_HandleTypeDef* Uart::UART(Uart* serial)
{
    return serial->uart;
}

void Uart::RxISR(Uart* serial, const uint16_t fifo_idx)
{
    // NOTE- Do NOT put a breakpoint in here, the callbacks will
    // get blocked will get memory access errors because the
    // callback on your breakpoint will throw off the values and then
    // will cause the value to be at the wrong idx and everything will go
    // ka-boom, crash, plop. Overflow errors and stuff.
    const uint16_t num_recv = fifo_idx - serial->rx_write_idx;
    // serial->UpdateRx(num_recv);
    serial->unread += num_recv;
    serial->rx_write_idx += num_recv;
    if (serial->rx_write_idx >= serial->rx_buff_sz)
    {
        serial->rx_write_idx = 0;
    }

    if (serial->unread > serial->rx_buff_sz)
    {
        // TODO
        // Error("SerialStm rx isr", "Overflowed rx buffer");
    }
}

void Uart::TxISR(Uart* serial)
{
    serial->UpdateTx();
}
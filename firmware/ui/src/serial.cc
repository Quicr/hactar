#include "serial.hh"

#include <memory.h>

#include "logger.hh"


// TODO unify into ui and net

Serial::Serial(UART_HandleTypeDef* uart, const uint16_t num_rx_packets,
    uint8_t& tx_buff, const uint32_t tx_buff_sz,
    uint8_t& rx_buff, const uint32_t rx_buff_sz):
    SerialHandler(num_rx_packets, tx_buff, tx_buff_sz, rx_buff, rx_buff_sz, Transmit, this),
    uart(uart)
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

void Serial::Stop()
{
    HAL_UART_AbortReceive_IT(uart);
}

void Serial::Reset()
{
    rx_write_idx = 0;
    rx_read_idx = 0;

    link_packet_t* packets = rx_packets.Buffer();
    for (int i = 0 ; i < rx_packets.Size(); ++i)
    {
        packets[i].is_ready = false;
    }

    packet = nullptr;
    bytes_read = 0;

    StartReceive();
}

void Serial::ResetRecv()
{
    uint16_t err = uart->Instance->SR;
    Stop();
    UNUSED(err);

    Reset();
}

void Serial::UpdateUnread(const uint16_t update)
{
    // UI_LOG_INFO("unread %d, update %d", (int)unread, (int)update);
    __disable_irq();
    unread -= update;
    __enable_irq();
    // UI_LOG_INFO("unread %d", (int)unread);
}

void Serial::Transmit(void* arg)
{
    Serial* self = static_cast<Serial*>(arg);
    HAL_UART_Transmit_DMA(self->uart, self->tx_buff + self->tx_read_idx, self->num_to_send);
}

const UART_HandleTypeDef* Serial::UART(Serial* serial)
{
    return serial->uart;
}

void Serial::RxISR(Serial* serial, const uint16_t fifo_idx)
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

void Serial::TxISR(Serial* serial)
{
    serial->UpdateTx();
}
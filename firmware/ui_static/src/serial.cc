#include "serial.hh"

#include <memory.h>

Serial::Serial(UART_HandleTypeDef* uart):
    uart(uart),
    tx_is_free(true),
    rx_buff{ 0 },
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

uint8_t Serial::Read()
{
    if (read_idx >= Rx_Buff_Sz)
    {
        read_idx = 0;
    }

    return rx_buff[read_idx++];
}

void Serial::Read(uint8_t* data, const size_t size, const size_t num_bytes)
{
    const size_t sz = size < num_bytes ? size : num_bytes;
    const size_t bytes = Rx_Buff_Sz < sz ? Rx_Buff_Sz : sz;
    const size_t total_bytes = bytes < unread ? bytes : unread;

    size_t remaining_bytes = total_bytes;
    size_t idx = 0;

    if (read_idx + remaining_bytes >= Rx_Buff_Sz)
    {
        const uint16_t read_space = Rx_Buff_Sz - read_idx;
        const uint16_t bytes = read_space < remaining_bytes ? read_space : remaining_bytes;

        memcpy(data, rx_buff+read_idx, sizeof(*data) * bytes);

        read_idx = 0;
        idx = bytes;
        remaining_bytes -= bytes;
    }

    memcpy(data+idx, rx_buff+read_idx, sizeof(*data) * remaining_bytes);
    read_idx += remaining_bytes;
    unread -= total_bytes;
}

void Serial::Read(uint8_t** data, size_t& num_bytes)
{
    const size_t remaining_space = Rx_Buff_Sz - read_idx;
    num_bytes = remaining_space < num_bytes ? remaining_space : num_bytes;
    num_bytes = num_bytes < unread ? num_bytes : unread;

    // Get the offset
    const size_t offset = Rx_Buff_Sz - num_bytes;

    // Set the param to the rx_buff + offset
    *data = rx_buff + offset;

    // Move the read head
    read_idx += num_bytes;

    if (read_idx >= Rx_Buff_Sz)
    {
        read_idx = 0;
    }
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

void Serial::RxEvent(const uint16_t idx)
{
    // NOTE- Do NOT put a breakpoint in here, the callbacks will
    // get blocked will get memory access errors because the
    // callback on your breakpoint will throw off the values and then
    // will cause the value to be at the wrong idx and everything will go
    // ka-boom, crash, plop. Overflow errors and stuff.
    const uint16_t num_recv = idx - write_idx;
    unread += num_recv;
    write_idx += num_recv;
    if (write_idx >= Rx_Buff_Sz)
    {
        write_idx = write_idx - Rx_Buff_Sz;
    }
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
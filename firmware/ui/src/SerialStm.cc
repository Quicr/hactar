#include "SerialStm.hh"
#include "main.hh"
#include "String.hh"

SerialStm::SerialStm(UART_HandleTypeDef* uart_handler,
                     unsigned short rx_ring_sz) :
    uart(uart_handler),
    rx_ring(rx_ring_sz),
    rx_buff(new unsigned char[Rx_Buff_Size]),
    rx_buff_idx(0),
    tx_free(true)
{
    StartRx();
}

SerialStm::~SerialStm()
{
    delete [] rx_buff;
}

size_t SerialStm::AvailableBytes()
{
    return rx_ring.AvailableBytes();
}

unsigned char SerialStm::Read()
{
    return rx_ring.Read();
}

bool SerialStm::ReadyToWrite()
{
    return tx_free;
}

void SerialStm::Write(unsigned char* buff, const unsigned short buff_sz)
{
    tx_free = false;
    uint8_t start_byte[1] = { 0xFF };
    HAL_UART_Transmit(uart, start_byte, 1, 1);

    HAL_UART_Transmit_IT(uart, buff, buff_sz);
}

// un-inherited functions
void SerialStm::RxEvent(uint16_t size)
{
    uint16_t next_bytes = size - rx_buff_idx;

    for (uint16_t i = 0; i < size; ++i)
    {
        rx_ring.Write(rx_buff[i]);
    }

    if (rx_ring.IsFull())
    {
        // Maybe set "busy?"
    }

    // Restart receive
    StartRx();
}

void SerialStm::StartRx()
{
    // Begin the receive IT
    HAL_UARTEx_ReceiveToIdle_IT(uart, rx_buff, Rx_Buff_Size);
}

void SerialStm::TxEvent()
{
    // With the event that means the transmission completed
    tx_free = true;
}
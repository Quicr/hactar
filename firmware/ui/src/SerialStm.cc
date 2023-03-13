#include "SerialStm.hh"

SerialStm::SerialStm(UART_HandleTypeDef* uart_handler,
                     unsigned short rx_ring_sz) :
    uart(uart_handler),
    rx_ring(rx_ring_sz),
    rx_buff(new unsigned char[1]),
    tx_free(true)
{
    StartRx();
}

unsigned long SerialStm::AvailableBytes()
{
    return rx_ring.AvailableBytes();
}

unsigned long SerialStm::Read()
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
    HAL_UART_Transmit_IT(uart, buff, buff_sz);
}

// un-inherited functions
void SerialStm::RxEvent()
{
    // Buffer only has one byte
    rx_ring.Write(rx_buff[0]);

    // Restart receive
    StartRx();
}

void SerialStm::StartRx()
{
    // Zero the buffer
    rx_buff[0] = 0;

    // Begin the receive IT
    HAL_UARTEx_ReceiveToIdle_IT(uart, rx_buff, 1);
}

void SerialStm::TxEvent()
{
    // With the event that means the transmission completed
    tx_free = true;
}
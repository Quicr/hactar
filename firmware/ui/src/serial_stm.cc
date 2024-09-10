#include "serial_stm.hh"
#include <string>

#include <string>

#include "main.h"
extern UART_HandleTypeDef huart1;

SerialStm::SerialStm(UART_HandleTypeDef* uart_handler,
                     unsigned short rx_ring_sz) :
    uart(uart_handler),
    rx_ring(rx_ring_sz),
    rx_activated(false),
    tx_free(true)
{
    StartRx();
}

SerialStm::~SerialStm()
{
    uart = nullptr;
}

size_t SerialStm::Unread()
{
    return rx_ring.Unread();
}

unsigned char SerialStm::Read()
{
    return rx_ring.Read();
}

bool SerialStm::TransmitReady()
{
    return tx_free;
}

void SerialStm::Transmit(unsigned char* buff, const unsigned short buff_sz)
{
    tx_free = false;
    // TODO remove start byte and send packet length first.
    uint8_t start_byte[1] = { 0xFF };
    HAL_UART_Transmit(uart, start_byte, 1, HAL_MAX_DELAY);

    // HAL_UART_Transmit_IT(uart, buff, buff_sz);
    HAL_UART_Transmit_DMA(uart, buff, buff_sz);
}

// un-inherited functions

void SerialStm::RxEvent(uint16_t received_idx)
{
    uint16_t num_received = received_idx - rx_ring.WriteIdx();
    rx_ring.UpdateWriteHead(num_received);
    // uint8_t recv_msg[] = "UI: Received from ESP\n\r";
    // // TODO dma
    // HAL_UART_Transmit(&huart1, recv_msg, sizeof(recv_msg)/sizeof(*recv_msg), HAL_MAX_DELAY);
}

void SerialStm::StartRx()
{
    // If rx is already running do nothing
    if (rx_activated)
    {
        return;
    }

    rx_activated = true;
    // Begin the receive IT
    HAL_UARTEx_ReceiveToIdle_DMA(uart, rx_ring.Buffer(), rx_ring.Size());
}

void SerialStm::Reset()
{
    // If rx is not running do nothing
    if (!rx_activated)
    {
        return;
    }

    rx_activated = false;

    HAL_UART_AbortReceive_IT(uart);
}

void SerialStm::TxEvent()
{
    // With the event that means the transmission completed
    tx_free = true;
}

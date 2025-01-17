#pragma once

#include "stm32.h"
#include "stm32f4xx_hal_uart.h"
#include "serial_interface.hh"
#include "ring_buffer.hh"

class SerialStm : public SerialInterface
{
public:
    SerialStm(UART_HandleTypeDef* uart_handler,
              unsigned short rx_ring_sz=256);
    ~SerialStm();

    size_t Unread() override;
    unsigned char Read() override;
    bool TransmitReady() override;
    void Transmit(unsigned char* buff, const unsigned short buff_sz) override;

    // un-inherited functions
    void RxEvent(uint16_t size);
    void TxEvent();
    void StartRx();
    void Reset();
private:

    UART_HandleTypeDef* uart;

    // rx
    RingBuffer<unsigned char> rx_ring;
    bool rx_activated;

    // tx
    volatile bool tx_free;
};
#pragma once

#include "stm32.h"
#include "stm32f4xx_hal_uart.h"
#include "SerialInterface.hh"
#include "RingBuffer.hh"

class SerialStm : public SerialInterface
{
public:
    SerialStm(UART_HandleTypeDef* uart_handler,
              unsigned short rx_ring_sz=256);
    ~SerialStm();

    unsigned long AvailableBytes() override;
    unsigned long Read() override;
    bool ReadyToWrite() override;
    void Write(unsigned char* buff, const unsigned short buff_sz) override;

    // un-inherited functions
    void RxEvent();
    void TxEvent();
private:
    void StartRx();

    UART_HandleTypeDef* uart;

    // rx
    RingBuffer<unsigned char> rx_ring;
    unsigned char* rx_buff;

    // tx
    volatile bool tx_free;
};
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

    size_t AvailableBytes() override;
    unsigned char Read() override;
    bool ReadyToWrite() override;
    void Write(unsigned char* buff, const unsigned short buff_sz) override;

    // un-inherited functions
    void RxEvent(uint16_t size);
    void TxEvent();
private:
    void StartRx();

    // Constant variables
    const uint16_t Rx_Buff_Size = 128;

    UART_HandleTypeDef* uart;

    // rx
    RingBuffer<unsigned char> rx_ring;
    uint8_t* rx_buff;
    uint16_t rx_buff_idx;

    // tx
    volatile bool tx_free;
};
#pragma once

#include "driver/uart.h"
#include "SerialInterface.hh"
#include "RingBuffer.hh"

#define BUFFER_SIZE 256

class SerialEsp : public SerialInterface
{
public:
    SerialEsp(uart_port_t uart,
        unsigned long long tx_pin,
        unsigned long long rx_pin,
        unsigned long long rts_pin,
        unsigned long long cts_pin,
        uart_config_t uart_config,
        size_t ring_buffer_size);
    ~SerialEsp();

    size_t AvailableBytes() override;
    unsigned char Read() override;
    bool ReadyToWrite() override;
    void Write(unsigned char* buff, const unsigned short buff_size) override;

    // un-inherited functions
    static void RxEvent(void* parameter);
    static void TxEvent(void* parameter);
private:
    uart_port_t uart;
    RingBuffer<unsigned char> rx_ring;
    bool tx_free;

    QueueHandle_t uart_queue;
};
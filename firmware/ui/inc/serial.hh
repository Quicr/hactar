#pragma once

#include "app_main.hh"

#include "ring_buffer.hh"
#include "link_packet_t.hh"

// TODO update write code to push the data into a large
// buffer, and then use the interrupt to trigger another send.

class Serial
{
public:

    Serial(UART_HandleTypeDef* uart, const uint16_t num_rx_packets,
        uint8_t& tx_buff, const uint32_t tx_buff_sz,
        uint8_t& rx_buff, const uint32_t rx_buff_sz);
    ~Serial();

    void StartReceive();
    void Stop();
    void Reset();

    link_packet_t* Read();
    void Write(const uint8_t data);
    void Write(const link_packet_t& packet);
    void Write(const uint8_t* data, const uint16_t size);
    uint16_t Unread();

    const UART_HandleTypeDef* UART();

    static void RxISR(Serial* self, const uint16_t idx);
    static void TxISR(Serial* self);
protected:
    static void Transmit(Serial* self);

    UART_HandleTypeDef* uart;
    RingBuffer<link_packet_t> rx_packets;

    uint8_t* tx_buff;
    uint32_t tx_buff_sz;
    uint8_t* rx_buff;
    uint32_t rx_buff_sz;

    uint16_t tx_write_idx;
    uint16_t tx_read_idx;
    bool tx_free;
    uint16_t unsent;
    uint16_t num_to_send;

    uint16_t rx_write_idx;
    uint16_t rx_read_idx;
    uint16_t unread;
};
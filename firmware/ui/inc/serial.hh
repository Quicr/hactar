#pragma once

#include "main.h"

#include "ring_buffer.hh"
#include "link_packet_t.hh"

class Serial
{
public:

    Serial(UART_HandleTypeDef* uart, const uint16_t num_rx_packets);
    ~Serial();

    void StartReceive();
    void Reset();

    link_packet_t* Read();
    void Write(const uint8_t data);
    void Write(const uint8_t* data, const size_t size);
    void Write(const link_packet_t& packet);
    size_t Unread();

    void UpdateReadHead(size_t amt);
    void RxEvent(const uint16_t idx);
    void TxEvent();
    bool IsFree();
    void Free();

protected:
    void ChangeFreeState();

    static constexpr size_t Rx_Buff_Sz = 2048;

    UART_HandleTypeDef* uart;
    RingBuffer<link_packet_t> rx_packets;
    volatile bool tx_is_free;
    uint8_t rx_buff[Rx_Buff_Sz];
    link_packet_t* rx_packet;
    uint16_t write_idx;
    uint16_t read_idx;
    uint16_t unread;
};
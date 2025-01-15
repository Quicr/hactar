#pragma once

#include "main.h"

#include "ring_buffer.hh"
#include "packet_t.hh"

class Serial
{
public:

    Serial(UART_HandleTypeDef* uart);
    ~Serial();

    void StartReceive();
    void Reset();
    uint8_t Read();
    void Read(uint8_t* data, const size_t size, const size_t num_bytes);
    void Read(uint8_t** data, size_t& num_bytes);
    void UpdateReadHead(size_t amt);
    void Write(const uint8_t data);
    void Write(const uint8_t* data, const size_t size);
    void Write(const packet_t& packet);
    size_t Unread();

    void RxEvent(const uint16_t idx);
    void TxEvent();
    bool IsFree();
    void Free();

protected:
    void ChangeFreeState();

    static constexpr size_t Rx_Buff_Sz = 2048;

    UART_HandleTypeDef* uart;
    volatile bool tx_is_free;
    bool packet_started;
    uint16_t bytes_read;
    uint8_t rx_buff[Rx_Buff_Sz];
    RingBuffer<packet_t> rx_packets;
    packet_t* rx_packet;
    uint32_t write_idx;
    uint32_t read_idx;
    uint32_t unread;
};
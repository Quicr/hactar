#pragma once

#include "main.h"

class Serial
{
public:
    enum Packet_Type: uint8_t
    {
        Null,
        Ready,
        Audio,
        Text,
        MLS,

    };

    Serial(UART_HandleTypeDef* uart);
    ~Serial();

    void StartReceive();
    void Reset();
    void ReadSerial(uint8_t* data, const size_t size, const size_t num_bytes);
    void ReadSerial(uint8_t** data, size_t& num_bytes);
    void UpdateReadHead(size_t amt);
    void WriteSerial(const uint8_t* data, const size_t size);
    size_t Unread();

    void RxEvent(const uint16_t idx);
    bool IsFree();
    void Free();

protected:
    static constexpr size_t Rx_Buff_Sz = 1024;

    UART_HandleTypeDef* uart;
    volatile bool tx_is_free;
    uint8_t rx_buff[Rx_Buff_Sz];
    uint32_t write_idx;
    uint32_t read_idx;
    uint32_t unread;
};
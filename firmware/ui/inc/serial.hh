#pragma once

#include "app_main.hh"
#include "link_packet_t.hh"
#include "ring_buffer.hh"
#include "serial_handler/serial_handler.hh"

class Serial : public SerialHandler
{
public:
    Serial(UART_HandleTypeDef* uart,
           const uint16_t num_rx_packets,
           uint8_t& tx_buff,
           const uint32_t tx_buff_sz,
           uint8_t& rx_buff,
           const uint32_t rx_buff_sz);
    ~Serial();

    void StartReceive();
    void Reset();
    void Stop();
    void ResetRecv();

    static const UART_HandleTypeDef* UART(Serial* serial);

    static void RxISR(Serial* serial, const uint16_t idx);
    static void TxISR(Serial* serial);

protected:
    void UpdateUnread(const uint16_t update) override;
    static void Transmit(void* arg);

    UART_HandleTypeDef* uart;
};
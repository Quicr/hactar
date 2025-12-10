#pragma once

#include "app_main.hh"
#include "link_packet_t.hh"
#include "ring_buffer.hh"
#include "serial/serial.hh"
#include "uart_handler/uart_handler.hh"

class Uart : public UartHandler
{
public:
    Uart(UART_HandleTypeDef* uart,
         const uint16_t num_rx_packets,
         uint8_t& tx_buff,
         const uint32_t tx_buff_sz,
         uint8_t& rx_buff,
         const uint32_t rx_buff_sz,
         const bool use_slip = true);
    ~Uart();

    void StartReceive();
    void Reset();
    void Stop();
    void ResetRecv();

    static const UART_HandleTypeDef* UART(Uart* serial);

    static void RxISR(Uart* serial, const uint16_t idx);
    static void TxISR(Uart* serial);

protected:
    void UpdateUnread(const uint16_t update) override;
    static void Transmit(void* arg);

    UART_HandleTypeDef* uart;
};
#pragma once

#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "hal/uart_ll.h"
#include "soc/interrupts.h"

#include "ring_buffer.hh"

#include "link_packet_t.hh"

class Serial
{
public:

    Serial(const uart_port_t port, uart_dev_t& uart, const periph_interrput_t intr_source,
        const uart_config_t uart_config, const int tx_pin, const int rx_pin,
        const int rts_pin, const int cts_pin,
        const uint32_t rx_isr_buff_sz, const uint32_t tx_isr_buff_sz,
        const uint32_t tx_rings, const uint32_t rx_rings);

    ~Serial();

    uint16_t NumReadyRxPackets();
    link_packet_t* Read();
    link_packet_t* Write();

    void TestWrite();

    int rx(int idx)
    {
        return rx_isr_buff[idx];
    }

    uint32_t num_rx_intr = 0; // REMOVME
    uint32_t num_rx_recv = 0; // REMOVME
private:
    static void WriteTask(void* param);
    static void ReadTask(void* param);
    static bool IRAM_ATTR Transmit(Serial* self);
    static void IRAM_ATTR ISRHandler(void* args);

    uart_port_t port;
    uart_dev_t& uart;
    const uint32_t rx_isr_buff_sz;
    uint8_t* rx_isr_buff;
    const uint32_t tx_isr_buff_sz;
    uint8_t* tx_isr_buff;
    RingBuffer<link_packet_t> tx_packets;
    RingBuffer<link_packet_t> rx_packets;

    uart_isr_handle_t isr_handle;
    uint32_t rx_isr_buff_write;
    uint32_t rx_isr_buff_read;
    uint32_t tx_isr_buff_write;
    uint32_t tx_isr_buff_read;
    uint32_t tx_unwritten;

    bool tx_free;
    int32_t num_to_write;
};
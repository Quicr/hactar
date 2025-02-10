#pragma once

#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_event.h"
#include "hal/uart_ll.h"
#include "soc/interrupts.h"

#include "ring_buffer.hh"

#include "link_packet_t.hh"

#include "serial_handler/serial_handler.hh"

class Serial : public SerialHandler
{
public:

    Serial(const uart_port_t port, uart_dev_t& uart, const periph_interrput_t intr_source,
        const uart_config_t uart_config, const int tx_pin, const int rx_pin,
        const int rts_pin, const int cts_pin,
        uint8_t& tx_buff, const uint32_t tx_buff_sz,
        uint8_t& rx_buff, const uint32_t rx_buff_sz,
        const uint32_t rx_rings);

    ~Serial();

    uint16_t NumReadyRxPackets();

private:
    static IRAM_ATTR void Transmit(void* arg);
    static IRAM_ATTR void ISRHandler(void* args);
    FORCE_INLINE_ATTR void RxHandler(Serial* arg);

    uart_port_t port;
    uart_dev_t& uart;

    uart_isr_handle_t isr_handle;
};
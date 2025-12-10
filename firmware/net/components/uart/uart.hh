#pragma once

#include "driver/uart.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "hal/uart_ll.h"
#include "link_packet_t.hh"
#include "ring_buffer.hh"
#include "soc/interrupts.h"
#include "uart_handler/uart_handler.hh"

class Uart : public UartHandler
{
public:
    Uart(const uart_port_t port,
         uart_dev_t& uart,
         TaskHandle_t& read_handle,
         const periph_interrput_t intr_source,
         const uart_config_t uart_config,
         const int tx_pin,
         const int rx_pin,
         const int rts_pin,
         const int cts_pin,
         uint8_t& tx_buff,
         const uint32_t tx_buff_sz,
         uint8_t& rx_buff,
         const uint32_t rx_buff_sz,
         const uint32_t rx_rings,
         const uint32_t driver_tx_size = 2048,
         const uint32_t driver_rx_size = 4096,
         const uint32_t driver_queue_size = 20,
         const bool use_queue_task = true,
         const bool use_slip = true);

    ~Uart();

    void BeginEventTask();

    uint16_t NumReadyRxPackets();

protected:
    void UpdateUnread(const uint16_t update) override;

private:
    static void Transmit(void* arg);

    static void ReadTask(void* args);
    static void QueueReadTask(void* args);

    uart_port_t port;
    uart_dev_t& uart;
    TaskHandle_t& read_handle;
    const uart_config_t uart_config;
    const int tx_pin;
    const int rx_pin;
    const int rts_pin;
    const int cts_pin;
    const uint32_t driver_tx_size;
    const uint32_t driver_rx_size;
    const uint32_t driver_queue_size;
    const bool use_queue_task;
    QueueHandle_t queue;
    TaskHandle_t uart_task_handle;
};

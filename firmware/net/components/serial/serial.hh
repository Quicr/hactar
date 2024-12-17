#pragma once

#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_event.h"

#include "ring_buffer.hh"

class Serial
{
public:
    Serial(const uart_port_t uart, QueueHandle_t& queue,
        const size_t tx_task_sz, const size_t rx_task_sz,
        const size_t tx_buff_sz, const size_t rx_buff_sz);

private:
    static void WriteTask(void* param);
    static void ReadTask(void* param);
    
    // TODO remove
    static const size_t audio_sz = 355;


    uart_port_t uart;
    QueueHandle_t& queue;

    bool tx_data_ready;
    bool synced;

    uint8_t* tx_buff;
    size_t tx_buff_sz;
    uint16_t tx_read_mod;
    uint16_t tx_write_mod;
    uint16_t num_to_send;
    size_t num_write;
    uint8_t* rx_buff;
    size_t rx_buff_sz;
    size_t rx_buff_write_idx;
    size_t num_recv;
};
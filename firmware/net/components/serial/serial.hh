#pragma once

#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_event.h"

#include "ring_buffer.hh"

#include "packet_t.hh"

class Serial
{
public:

    Serial(const uart_port_t uart, QueueHandle_t& queue,
        const size_t tx_task, const size_t rx_task,
        const size_t tx_rings, const size_t rx_rings);

    uint16_t NumReadyRxPackets();
    packet_t* Read();
    packet_t* Write();

private:
    static void WriteTask(void* param);
    static void ReadTask(void* param);

    void HandleRxEvent(uart_event_t& event);
    void HandleRxData();

    uart_port_t uart;
    QueueHandle_t& queue;

    RingBuffer<packet_t> tx_packets;
    RingBuffer<packet_t> rx_packets;

    uint32_t num_recv;
    uint32_t num_sent;
};
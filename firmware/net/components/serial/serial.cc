#include "serial.hh"
#include "link_packet_t.hh"
#include "packet_builder.hh"

#include "esp_log.h"

Serial::Serial(const uart_port_t uart, QueueHandle_t& queue,
    const size_t tx_task_sz, const size_t rx_task_sz,
    const size_t tx_rings, const size_t rx_rings):
    uart(uart),
    queue(queue),
    tx_packets(tx_rings),
    rx_packets(rx_rings),
    num_recv(0),
    num_sent(0)
{
    // Start the tasks
    xTaskCreate(ReadTask, "serial_read_task", rx_task_sz, this, 1, NULL);
    xTaskCreate(WriteTask, "serial_write_task", tx_task_sz, this, 1, NULL);
}

link_packet_t* Serial::Read()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    link_packet_t* packet = &rx_packets.Read();
    packet->is_ready = false;
    return packet;
}

#if 1
void Serial::WriteTask(void* param)
{
    Serial* self = (Serial*)param;
    link_packet_t* tx_packet = nullptr;

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (!self->tx_packets.Peek().is_ready)
        {
            continue;
        }

        // Write it to serial
        tx_packet = &self->tx_packets.Read();

        uart_wait_tx_done(self->uart, 5 / portTICK_PERIOD_MS);

        uart_write_bytes(self->uart, tx_packet->data, tx_packet->length + Packet_Header_Size);

        tx_packet->is_ready = false;
        ++self->num_sent;
        // ESP_LOGI("uart tx", "serial packet sent %lu", self->num_sent);

    }
}
#else

// Loopback version
void Serial::WriteTask(void* param)
{
    Serial* self = (Serial*)param;
    link_packet_t* tx_packet = nullptr;

    while (1)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (!self->rx_packets.Peek().is_ready)
        {
            continue;
        }

        // Write it to serial
        tx_packet = &self->rx_packets.Read();

        uart_wait_tx_done(self->uart, 5 / portTICK_PERIOD_MS);

        uart_write_bytes(self->uart, tx_packet->data, tx_packet->length + Packet_Header_Size);

        tx_packet->is_ready = false;
        ++self->num_sent;
        ESP_LOGI("uart tx", "serial packet sent %lu", self->num_sent);
    }
}
#endif

link_packet_t* Serial::Write()
{
    return &tx_packets.Write();
}

void Serial::ReadTask(void* param)
{
    Serial* self = (Serial*)param;
    ESP_LOGI("uart rx", "Started!!!");

    uart_event_t event;
    static constexpr size_t Local_Buff_Size = 2048;
    uint8_t buff[Local_Buff_Size];
    link_packet_t* packet = nullptr;

    uint32_t total_recv = 0;
    uint16_t bytes_read = 0;

    size_t buffered_bytes = 1;
    int num_bytes = 0;

    while (1)
    {
        // Wait for an event in the queue
        if (!xQueueReceive(self->queue, (void*)&event, portMAX_DELAY))
        {
            continue;
        }
        switch (event.type)
        {
            case UART_DATA:
            {
                uart_get_buffered_data_len(self->uart, &buffered_bytes);

                // If the number of buffered bytes is less than 2 and
                // we haven't started a packet we want to wait a bit longer.
                // Otherwise, on every byte we want to read
                if (buffered_bytes <= 0)
                {
                    continue;
                }

                // ESP_LOGW("uart rx", "buffered bytes %zu", buffered_bytes);

                const uint32_t to_recv = buffered_bytes < Local_Buff_Size ? buffered_bytes : Local_Buff_Size;
                num_bytes = uart_read_bytes(self->uart, buff, to_recv, portMAX_DELAY);
                total_recv += num_bytes;

                self->num_recv += BuildPacket(buff, num_bytes, self->rx_packets);
                // ESP_LOGI("uart tx", "serial packet sent %lu", self->num_recv);
                break;
            }
            case UART_FIFO_OVF: // FIFO overflow
            {

                printf("UART FIFO Overflow!\n");
                uart_flush_input(self->uart); // Flush the input to recover
                xQueueReset(self->queue);   // Reset the event queue
                break;
            }
            case UART_BUFFER_FULL: // RX buffer full
            {

                printf("UART Buffer Full!\n");
                uart_flush_input(self->uart); // Flush the input to recover
                xQueueReset(self->queue);   // Reset the event queue
                break;
            }
            case UART_PARITY_ERR: // Parity error
            {

                printf("UART Parity Error!\n");
                uart_flush_input(self->uart); // Flush the input to recover
                xQueueReset(self->queue);   // Reset the event queue
                break;
            }
            case UART_FRAME_ERR: // Frame error
            {

                printf("UART Frame Error!\n");
                uart_flush_input(self->uart); // Flush the input to recover
                xQueueReset(self->queue);   // Reset the event queue
                break;
            }
            default:
            {

                printf("Unhandled UART event type: %d\n", event.type);
                uart_flush_input(self->uart); // Flush the input to recover
                xQueueReset(self->queue);   // Reset the event queue
                break;
            }

        }
    }
}

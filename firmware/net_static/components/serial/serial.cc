#include "serial.hh"

#include "esp_log.h"

Serial::Serial(const uart_port_t uart, QueueHandle_t& queue,
    const size_t tx_task_sz, const size_t rx_task_sz,
    const size_t tx_rings, const size_t rx_rings):
    audio_packets_recv(0),
    uart(uart),
    queue(queue),
    tx_data_ready(false),
    synced(true),
    tx_packets(tx_rings),
    rx_packets(rx_rings)
{
    // Start the tasks
    xTaskCreate(ReadTask, "serial_read_task", rx_task_sz, this, 1, NULL);
    xTaskCreate(WriteTask, "serial_write_task", tx_task_sz, this, 1, NULL);
}

Serial::packet_t* Serial::GetReadyRxPacket()
{
    if (!rx_packets.Peek().is_ready)
    {
        return nullptr;
    }

    packet_t* packet = &rx_packets.Read();
    packet->is_ready = false;
    return packet;
}

void Serial::WriteTask(void* param)
{
    Serial* self = (Serial*)param;
    packet_t* tx_packet = nullptr;
    self->audio_packets_sent = 0;


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

        uart_write_bytes(self->uart, tx_packet->data, tx_packet->length);

        tx_packet->is_ready = false;
        ++self->audio_packets_sent;
        ESP_LOGI("uart tx", "audio serial packet sent %lu", self->audio_packets_sent);

    }
}

// Loopback version
// void Serial::WriteTask(void* param)
// {
//     Serial* self = (Serial*)param;
//     packet_t* tx_packet = nullptr;
//     self->audio_packets_sent = 0;

//     while (1)
//     {
//         vTaskDelay(10 / portTICK_PERIOD_MS);

//         if (!self->rx_packets.Peek().is_ready)
//         {
//             continue;
//         }

//         // Write it to serial
//         tx_packet = &self->rx_packets.Read();

//         uart_wait_tx_done(self->uart, 5 / portTICK_PERIOD_MS);

//         uart_write_bytes(self->uart, tx_packet->data, tx_packet->length);

//         tx_packet->is_ready = false;
//         ++self->audio_packets_sent;
//     }
// }

Serial::packet_t* Serial::Write()
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
    packet_t* packet = nullptr;

    uint32_t total_recv = 0;
    uint16_t bytes_read = 0;

    size_t buffered_bytes = 1;
    int num_bytes = 0;

    while (1)
    {
        // Wait for an event in the queue
        if (!xQueueReceive(self->queue, (void*)&event, 10 / portTICK_PERIOD_MS))
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
                uint32_t idx = 0;

                while (idx < num_bytes)
                {
                    if (packet == nullptr)
                    {
                        // Reset the number of bytes read for our next packet
                        bytes_read = 0;

                        packet = &self->rx_packets.Write();
                        packet->is_ready = false;

                        // Get the length
                        // Little endian format
                        // We use idx, because if we are already partially
                        // through the buffered data we want to grab those 
                        // next set of bytes.
                        while (bytes_read < 2 && idx < num_bytes)
                        {
                            packet->data[bytes_read++] = buff[idx++];
                        }
                    }
                    else if (bytes_read < 2)
                    {
                        // If we get here then packet has been set to something 
                        // other than nullptr and only one byte has been 
                        // read which is insufficient to compare against the len
                        packet->data[bytes_read++] = buff[idx++];
                        continue;
                    }
                    else
                    {
                        // We can copy bytes until we run out of space 
                        // or out of buffered bytes
                        while (bytes_read < packet->length && idx < num_bytes)
                        {
                            packet->data[bytes_read] = buff[idx];
                            ++bytes_read;
                            ++idx;
                        }

                        if (bytes_read >= packet->length)
                        {
                            // Done the packet
                            packet->is_ready = true;

                            // TODO make a task that checks packets ??
                            // Determine what to do with it
                            switch (packet->type)
                            {
                                case Packet_Type::Ready:
                                {
                                    // ready message ignore for now
                                    break;
                                }
                                case Packet_Type::Audio:
                                {
                                    ++self->audio_packets_recv;
                                    ESP_LOGI("uart rx", "audio packet recv %lu, packet len %u", self->audio_packets_recv, packet->length);

                                    break;
                                }
                                default:
                                {
                                    break;
                                }
                            }

                            // Null out our packet pointer
                            packet = nullptr;
                        }
                    }
                }
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

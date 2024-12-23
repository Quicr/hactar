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

    // const uint16_t tx_buff_sz_2 = self->tx_buff_sz / 2;

    while (1)
    {
        if (self->tx_data_ready)
        {
            // uint16_t offset = self->tx_read_mod * tx_buff_sz_2;
            // uart_write_bytes(self->uart, self->tx_buff + offset, self->num_to_send);
            // self->tx_data_ready = false;

            // ++self->num_write;

            // // ESP_LOGW("uart tx", "transmit bytes: %u, recv %zu sent %zu", self->num_to_send, self->num_recv, self->num_write);
            // if (self->num_write != self->num_recv)
            // {
            //     self->synced = false;
            //     while (true)
            //     {
            //         ESP_LOGE("UART", "Failed to stay synced recv %zu sent %zu", self->num_recv, self->num_write);
            //         vTaskDelay(5000 / portTICK_PERIOD_MS);
            //     }
            // }
            // uart_wait_tx_done(self->uart, portMAX_DELAY);
            // self->tx_read_mod = !self->tx_read_mod;
        }
        else
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }

}

void Serial::ReadTask(void* param)
{
    Serial* self = (Serial*)param;
    ESP_LOGI("uart rx", "Started!");

    uart_event_t event;

    while (1)
    {
        // Wait for an event in the queue
        if (!xQueueReceive(self->queue, (void*)&event, 10 / portTICK_PERIOD_MS))
        {
            continue;
        }

        // TODO REMOVE ME
        while (!self->synced)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        self->HandleRxEvent(event);
    }
}

void Serial::HandleRxEvent(uart_event_t event)
{
    switch (event.type)
    {
        case UART_DATA:
        {
            HandleRxData();
            break;
        }
        case UART_FIFO_OVF: // FIFO overflow
        {

            printf("UART FIFO Overflow!\n");
            uart_flush_input(uart); // Flush the input to recover
            xQueueReset(queue);   // Reset the event queue
            break;
        }
        case UART_BUFFER_FULL: // RX buffer full
        {

            printf("UART Buffer Full!\n");
            uart_flush_input(uart); // Flush the input to recover
            xQueueReset(queue);   // Reset the event queue
            break;
        }
        case UART_PARITY_ERR: // Parity error
        {

            printf("UART Parity Error!\n");
            uart_flush_input(uart); // Flush the input to recover
            xQueueReset(queue);   // Reset the event queue
            break;
        }
        case UART_FRAME_ERR: // Frame error
        {

            printf("UART Frame Error!\n");
            uart_flush_input(uart); // Flush the input to recover
            xQueueReset(queue);   // Reset the event queue
            break;
        }
        default:
        {

            printf("Unhandled UART event type: %d\n", event.type);
            uart_flush_input(uart); // Flush the input to recover
            xQueueReset(queue);   // Reset the event queue
            break;
        }

    }
}

void Serial::HandleRxData()
{
    static constexpr size_t Local_Buff_Size = 512;
    static uint8_t buff[Local_Buff_Size];
    static packet_t* packet = nullptr;
    static bool partial_packet = false;

    static uint32_t total_recv = 0;
    static uint16_t bytes_read = 0;
    
    size_t buffered_bytes = 0;
    int num_bytes = 0;
    // TODO should I put a while loop on top of getting buffered data until we get
    // zero bytes??

    // By default the max length is 120-121 (timing stuff) as the
    // default fifo buffer size is 128 and an interrupt is sent at
    // 120
    uart_get_buffered_data_len(uart, &buffered_bytes);

    // If the number of buffered bytes is less than 2 and 
    // we haven't started a packet we want to wait a bit longer.
    // Otherwise, on every byte we want to read
    // ESP_LOGW("uart rx", "buffered bytes %zu", buffered_bytes);
    if (buffered_bytes < 2 && !partial_packet)
    {
        return;
    }
    else if (buffered_bytes <= 0)
    {
        return;
    }

    const size_t to_recv = std::min(buffered_bytes, Local_Buff_Size);

    num_bytes = uart_read_bytes(uart, buff, buffered_bytes, portMAX_DELAY);

    total_recv += num_bytes;
    // ESP_LOGI("uart rx", "packet status %d, num_bytes: %d total received %lu", (int)partial_packet, num_bytes, total_recv);

    uint32_t idx = 0;

    while (num_bytes > 0)
    {
        if (!partial_packet)
        {
            // Start the packet
            partial_packet = true;

            // Get the packet's pointer
            // TODO test that this is viable C++
            packet = &rx_packets.Write();

            // Get the length
            // Little endian format
            // We use idx, because if we are already partially through the 
            // data we want to grab those next set of bytes.
            packet->length = buff[idx + 0];
            packet->length |= buff[idx + 1] << 8;
            // ESP_LOGI("uart rx", "packet length %u", packet->length);

            num_bytes -= 2;
            bytes_read += 2;
            idx += 2;
        }

        // Just copy the bytes
        while (bytes_read < packet->length && num_bytes > 0)
        {
            packet->data[bytes_read++] = buff[idx++];
            --num_bytes;
        }

        if (bytes_read >= packet->length)
        {
            // ESP_LOGI("uart rx", "packet len: %u type: %d", packet->length, (int)packet->type);

            // Done the packet
            packet->is_ready = true;
            partial_packet = false;
            bytes_read = 0;

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
                    ++audio_packets_recv;
                    ESP_LOGI("uart rx", "audio packet recv %lu", audio_packets_recv);

                    break;
                }
                default:
                {
                    break;
                }
            }
        }
    }
}
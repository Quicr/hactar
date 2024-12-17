#include "serial.hh"

#include "esp_log.h"

Serial::Serial(const uart_port_t uart, QueueHandle_t& queue,
    const size_t tx_task_sz, const size_t rx_task_sz,
    const size_t tx_buff_sz, const size_t rx_buff_sz):
    uart(uart),
    queue(queue),
    tx_data_ready(false),
    synced(true),
    tx_buff(new uint8_t[tx_buff_sz]{0}),
    tx_buff_sz(tx_buff_sz),
    tx_read_mod(0),
    tx_write_mod(0),
    num_to_send(0),
    num_write(0),
    rx_buff(new uint8_t[rx_buff_sz]{0}),
    rx_buff_sz(rx_buff_sz),
    num_recv(0)
{
    // Start the tasks
    xTaskCreate(ReadTask, "serial_read_task", rx_task_sz, this, 1, NULL);
    xTaskCreate(WriteTask, "serial_write_task", tx_task_sz, this, 1, NULL);
}

void Serial::WriteTask(void* param)
{
    Serial* self = (Serial*)param;

    const uint16_t tx_buff_sz_2 = self->tx_buff_sz / 2;

    while (1)
    {
        if (self->tx_data_ready)
        {
            uint16_t offset = self->tx_read_mod * tx_buff_sz_2;
            uart_write_bytes(self->uart, self->tx_buff + offset, self->num_to_send);
            self->tx_data_ready = false;

            ++self->num_write;

            ESP_LOGW("uart tx", "transmit bytes: %u, recv %zu sent %zu", self->num_to_send, self->num_recv, self->num_write);
            if (self->num_write != self->num_recv)
            {
                self->synced = false;
                while (true)
                {
                    ESP_LOGE("UART", "Failed to stay synced recv %zu sent %zu", self->num_recv, self->num_write);
                    vTaskDelay(5000 / portTICK_PERIOD_MS);
                }
            }
            uart_wait_tx_done(self->uart, portMAX_DELAY);
            self->tx_read_mod = !self->tx_read_mod;
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

    const uint16_t tx_buff_sz_2 = self->tx_buff_sz / 2;
    
    uart_event_t event;
    uint8_t data[1024];

    uint32_t total_recv = 0;
    uint32_t events_recv = 0;

    bool partial_packet = false;
    uint16_t packet_len = 0;
    uint8_t packet_type = 0;
    uint16_t bytes_recv = 0;

    uint16_t rx_buff_write_idx = 0;

    size_t bytes_buffered = 0;
    int num_bytes = 0;
    int num_bytes_read = 0;

    ESP_LOGI("uart rx", "Started!");


    while (1)
    {
        // Wait for an event in the queue
        if (!xQueueReceive(self->queue, (void*)&event, 10 / portTICK_PERIOD_MS))
        {
            continue;
        }

        while (!self->synced)
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

        switch (event.type)
        {
            case UART_DATA:
            {
                // By default the max length is 120-121 (timing stuff) as the
                // default fifo buffer size is 128 and an interrupt is sent at
                // 120
                uart_get_buffered_data_len(self->uart, &bytes_buffered);
                if (bytes_buffered <= 0)
                {
                    continue;
                }

                num_bytes = uart_read_bytes(self->uart, data, bytes_buffered, portMAX_DELAY);
                ++events_recv;


                total_recv += num_bytes;
                // ESP_LOGI("uart rx", "packet status %d, received %lu, events %lu ", (int)partial_packet, total_recv, events_recv);

                uint32_t idx = 0;

                while (num_bytes > 0)
                {
                    if (!partial_packet)
                    {
                        // Start the packet
                        partial_packet = true;

                        // Get the length
                        packet_len = static_cast<uint16_t>(data[idx + 0]) << 8 | data[idx + 1];

                        // Get the type
                        packet_type = data[idx + 2];
                        // idx = 2;

                        // ESP_LOGI("uart rx", "packet len: %u type: %d", packet_len, (int)packet_type);
                    }

                    // Just copy the bytes
                    while (bytes_recv < packet_len && num_bytes > 0)
                    {
                        self->rx_buff[rx_buff_write_idx++] = data[idx++];
                        ++bytes_recv;
                        --num_bytes;
                    }

                    // HACK This is all hacky
                    if (bytes_recv >= packet_len)
                    {
                        bytes_recv = 0;
                        // Done the packet
                        partial_packet = false;

                        // Determine what to do with it
                        switch (packet_type)
                        {
                            case 1:
                            {
                                // ready message ignore for now
                                break;
                            }
                            case 2:
                            {

                                int tx_write_offset = self->tx_write_mod * tx_buff_sz_2;

                                // Audio copy to the tx buffer
                                for (int i = 0; i < Serial::audio_sz; ++i)
                                {
                                    self->tx_buff[tx_write_offset + i] = self->rx_buff[i];
                                }

                                self->tx_write_mod = !self->tx_write_mod;
                                self->num_to_send = audio_sz;
                                self->tx_data_ready = true;
                                ++self->num_recv;
                                rx_buff_write_idx = 0;

                                break;
                            }
                            default:
                            {
                                num_bytes--;
                            }
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
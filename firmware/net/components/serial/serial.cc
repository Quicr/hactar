#include "serial.hh"
#include "logger.hh"
#include <random>

// This code is VERY specifically using espressif's serial event system which
// has many flaws, including but not limited to taking forever and less
// granularity.

Serial::Serial(const uart_port_t port,
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
               const uint32_t rx_rings) :
    SerialHandler(rx_rings, tx_buff, tx_buff_sz, rx_buff, rx_buff_sz, Transmit, this),
    port(port),
    read_handle(read_handle),
    uart_task_handle(nullptr)
{
    ESP_ERROR_CHECK(uart_param_config(port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(port, tx_pin, rx_pin, rts_pin, cts_pin));
    ESP_ERROR_CHECK(uart_driver_install(port, 4096, 2048, 20, &queue, 0));

    xTaskCreate(EventTask, "UART Event Task", 4096, this, 10, &uart_task_handle);
}

Serial::~Serial()
{
    if (uart_task_handle)
    {
        vTaskDelete(uart_task_handle);
    }

    uart_driver_delete(port);
}

void Serial::UpdateUnread(const uint16_t update)
{
    unread -= update;
}

void Serial::Transmit(void* arg)
{
    // TODO semaphores?
    Serial* serial = static_cast<Serial*>(arg);
    uart_write_bytes(serial->port, serial->tx_buff + serial->tx_read_idx, serial->num_to_send);
    serial->tx_free = true;
    serial->UpdateTx();
}

void Serial::EventTask(void* arg)
{
    Serial* serial = static_cast<Serial*>(arg);
    uart_event_t event;

    while (true)
    {
        if (!xQueueReceive(serial->queue, &event, portMAX_DELAY))
        {
            continue;
        }

        // NET_LOG_INFO("got event, size %d", event.size);

        switch (event.type)
        {
        case UART_DATA:
        {
            int space_remain;
            int bytes_to_read;
            int num_bytes = 0;
            int total_read = 0;
            while (total_read < event.size)
            {
                space_remain = serial->rx_buff_sz - serial->rx_write_idx;

                bytes_to_read = space_remain < event.size ? space_remain : event.size;

                num_bytes = uart_read_bytes(serial->port, serial->rx_buff + serial->rx_write_idx,
                                            bytes_to_read, 0);

                if (num_bytes == -1)
                {
                    continue;
                }

                total_read += num_bytes;

                serial->rx_write_idx += num_bytes;
                if (serial->rx_write_idx >= serial->rx_buff_sz)
                {
                    serial->rx_write_idx = 0;
                }
            }
            serial->unread += total_read;

            xTaskNotifyGive(serial->read_handle);

            break;
        }
        case UART_FIFO_OVF:
        {

            ESP_LOGW("SerialPort", "FIFO Overflow detected");
            uart_flush_input(serial->port);
            xQueueReset(serial->queue);
            break;
        }
        case UART_BUFFER_FULL:
        {
            ESP_LOGW("SerialPort", "Ring buffer full");
            uart_flush_input(serial->port);
            xQueueReset(serial->queue);
            break;
        }
        case UART_PARITY_ERR:
        {

            ESP_LOGE("SerialPort", "Parity error");
            break;
        }
        case UART_FRAME_ERR:
        {

            ESP_LOGE("SerialPort", "Frame error");
            break;
        }
        default:
        {
            ESP_LOGI("SerialPort", "Unhandled UART event type: %d", event.type);
            break;
        }
        }
    }
}

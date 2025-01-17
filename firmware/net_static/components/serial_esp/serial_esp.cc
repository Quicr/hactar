#include "serial_esp.hh"
#include "error.hh"

#include "logger.hh"

SerialEsp::SerialEsp(uart_port_t uart,
    unsigned long long tx_pin,
    unsigned long long rx_pin,
    unsigned long long rts_pin,
    unsigned long long cts_pin,
    uart_config_t uart_config,
    size_t ring_buff_size) :
    uart(uart),
    rx_ring(ring_buff_size),
    tx_free(true),
    queue(nullptr)
{
    esp_err_t res;
    // NOTE rx buff is MINIMUM 128
    // res = uart_driver_install(uart, BUFFER_SIZE * 4, BUFFER_SIZE * 4, 20, &queue, 0);
    // printf("install res=%d\n", res);
    // res = uart_set_pin(uart, tx_pin, rx_pin, rts_pin, cts_pin);
    // printf("uart set pin res=%d\n", res);
    // res = uart_param_config(uart, &uart_config);
    // printf("install res=%d\n", res);
    // xTaskCreate(RxEvent, "uart_event_task", 8192, (void*)this, 0, NULL);
}

SerialEsp::~SerialEsp()
{

}

size_t SerialEsp::Unread()
{
    return rx_ring.Unread();
}

unsigned char SerialEsp::Read()
{
    return rx_ring.Read();
}

bool SerialEsp::TransmitReady()
{
    return tx_free;
}

void SerialEsp::Transmit(unsigned char* buff, const unsigned short buff_size)
{
    // Wait until the previous message is sent
    while (uart_wait_tx_done(uart, 100))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    uint8_t start[] = {0xFF};
    uart_write_bytes(uart, start, 1);

    // Wait until the previous message is sent
    while (uart_wait_tx_done(uart, 1))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    // Send the whole message
    uart_write_bytes(uart, buff, buff_size);
}

// UART Events
void SerialEsp::RxEvent(void* parameter)
{
    SerialEsp* serial = (SerialEsp*)parameter;

    uart_event_t event;
    unsigned char buff[BUFFER_SIZE];

    while (true)
    {
        if (!xQueueReceive(serial->queue, (void*)&event, 1 / portTICK_PERIOD_MS))
        {
            continue;
        }

        printf("net-serial[%d]: Event size: %d, type: %d\n", serial->uart, event.size, event.type);

        switch (event.type)
        {
            case UART_DATA:
            {
                size_t write_idx;
                size_t space_remain;
                size_t len_to_read;

                unsigned char* ring_buff;

                int num_bytes = 0;
                int total_read = 0;
                while (total_read < event.size)
                {
                    write_idx = serial->rx_ring.WriteIdx();

                    space_remain = serial->rx_ring.Size() - write_idx;

                    len_to_read = space_remain < event.size ? space_remain : event.size;

                    ring_buff = serial->rx_ring.Buffer() + write_idx;
                    num_bytes = uart_read_bytes(serial->uart, ring_buff, len_to_read, 10 / portTICK_PERIOD_MS);

                    if (num_bytes < 0)
                    {
                        break;
                    }

                    total_read += num_bytes;

                    serial->rx_ring.UpdateWriteHead(num_bytes);
                }

                break;
            }
            case UART_FIFO_OVF:
            {
                // Fifo overflow. If this happens we should add flow control
                uart_flush_input(serial->uart);
                vTaskDelete(NULL);
                ErrorState("Hardware FIFO overflow", 0, 0, 1);
                break;
            }
            case UART_BUFFER_FULL:
            {
                // Some how we got full before we read, we should probably
                // read more often OR increase the buffer size
                uart_flush_input(serial->uart);
                vTaskDelete(NULL);
                ErrorState("UART Buffer full", 0, 1, 1);
                break;
            }
            case UART_BREAK:
            {
                vTaskDelete(NULL);
                ErrorState("UART rx break detected", 1, 0, 1);
                break;
            }
            case UART_PARITY_ERR:
            {
                vTaskDelete(NULL);
                ErrorState("UART parity error", 1, 1, 0);
                break;
            }
            case UART_FRAME_ERR:
            {
                vTaskDelete(NULL);
                ErrorState("UART parity error", 1, 0, 0);
                break;
            }
            case UART_PATTERN_DET:
            {
                vTaskDelete(NULL);
                ErrorState("UART pattern detected somehow", 1, 1, 1);
                break;
            }
            default:
            {
                vTaskDelete(NULL);
                ErrorState("Unhandled event detected", 1, 1, 1);
                break;
            }
        }
    }

    vTaskDelete(NULL);
}

void SerialEsp::TxEvent(void* parameter)
{
    ((SerialEsp*)parameter)->tx_free = true;
}
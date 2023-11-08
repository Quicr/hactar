#include "SerialEsp.hh"
#include "Error.hh"

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
    uart_queue(nullptr)
{
    esp_err_t res;
    // NOTE rx buff is MINIMUM 128
    res = uart_driver_install(uart, BUFFER_SIZE * 2, BUFFER_SIZE * 2, 20, &uart_queue, 0);
    printf("install res=%d\n", res);
    res = uart_set_pin(uart, tx_pin, rx_pin, rts_pin, cts_pin);
    printf("uart set pin res=%d\n", res);
    res = uart_param_config(uart, &uart_config);
    printf("install res=%d\n", res);
    xTaskCreate(RxEvent, "uart_event_task", 4096, (void*)this, 12, NULL);
}

SerialEsp::~SerialEsp()
{

}

size_t SerialEsp::AvailableBytes()
{
    return rx_ring.AvailableBytes();
}

unsigned char SerialEsp::Read()
{
    return rx_ring.Read();
}

bool SerialEsp::ReadyToWrite()
{
    return tx_free;
}

void SerialEsp::Write(unsigned char* buff, const unsigned short buff_size)
{
    // Wait until the previous message is sent
    while (uart_wait_tx_done(uart, 100))
    {
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    printf("NET: Send to ui");
    for (int i =0; i <  buff_size; ++i )
    {
        printf("%d", (int)buff[i]);
    }
    printf("\n\r");
    uint8_t start[] = {0xFF};
    uart_write_bytes(uart, start, 1);

    // Wait until the previous message is sent
    while (uart_wait_tx_done(uart, 100))
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
    printf("Serial %d\n", serial->uart);

    uart_event_t event;
    unsigned char buff[BUFFER_SIZE];

    while (true)
    {
        if (!xQueueReceive(serial->uart_queue, (void*)&event, portMAX_DELAY))
            continue;

        printf("NET-serial: uart[%d] event size: %d, type: %d\n", serial->uart, event.size, event.type);

        switch (event.type)
        {
            case UART_DATA:
            {
                printf("NET-serial: read serial bytes\n");
                uart_read_bytes(serial->uart, buff, event.size, portMAX_DELAY);
                printf("NET-serial: Send to ring buffer\n");

                for (size_t i = 0; i < event.size; ++i)
                {
                    printf("%d ", (int)buff[i]);
                    serial->rx_ring.Write(buff[i]);
                }
                printf("\n\r");


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
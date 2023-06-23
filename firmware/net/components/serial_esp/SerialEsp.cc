#include "SerialEsp.hh"

SerialEsp::SerialEsp(uart_port_t uart,
                     unsigned long long tx_pin,
                     unsigned long long rx_pin,
                     unsigned long long rts_pin,
                     unsigned long long cts_pin,
                     uart_config_t uart_config,
                     size_t ring_buff_size) :
    uart(uart),
    rx_ring(ring_buff_size)
{
    esp_err_t res;
    // NOTE rx buff is MINIMUM 128
    res = uart_driver_install(uart, BUFFER_SIZE * 2, BUFFER_SIZE * 2, 20, &uart_queue, 0);
    printf("install res=%d\n", res);
    res = uart_set_pin(uart, tx_pin, rx_pin, rts_pin, cts_pin);
    printf("uart set pin res=%d\n", res);
    res = uart_param_config(uart, &uart_config);
    printf("install res=%d\n", res);
    xTaskCreate(RxEvent, "uart_event_task", 2048, (void*)this, 12, NULL);
}

SerialEsp::~SerialEsp()
{
}

unsigned long SerialEsp::AvailableBytes()
{
    // return uart->available();
    return 0;
}

unsigned long SerialEsp::Read()
{
    // Read from the uart
    return 0;
}

bool SerialEsp::ReadyToWrite()
{
    // return uart->availableForWrite();

    return 0;
}

void SerialEsp::Write(unsigned char* buff, const unsigned short buff_size)
{
    // digitalWrite(5, LOW);
    // uart->write(0xFF);
    // uart->write(buff, buff_size);
    // digitalWrite(5, HIGH);
}

// UART Events
void SerialEsp::RxEvent(void* parameter)
{
    SerialEsp* serial = (SerialEsp*)parameter;
    printf("Serial %d\n", serial->uart);

    uart_event_t event;
    size_t num_buffered;
    // TODO use ring buff
    unsigned char* buff = new unsigned char[BUFFER_SIZE];

    while (true)
    {

        // TODO move to ISR
        if (!xQueueReceive(serial->uart_queue, (void*)&event, 0xFF))
            continue;

        printf("uart[%d] event size: %d\n", serial->uart, event.size);


        uart_read_bytes(serial->uart, buff, event.size, 0xFF);

        for (int i = 0; i < event.size; i++)
        {
            printf("buffer[%d] = %d", i, (int)buff[i]);
        }
        printf("\nMessage: ");

        for (int i = 0; i < event.size; i++)
        {
            printf("%c", buff[i]);
        }
        printf("\n");

        switch (event.type)
        {
            default:
                break;
        }

    }

    delete buff;
}

void SerialEsp::TxEvent(void* parameter)
{

}
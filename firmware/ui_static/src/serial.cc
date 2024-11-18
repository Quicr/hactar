#include "serial.hh"

Serial::Serial(UART_HandleTypeDef* uart) :
    uart(uart),
    is_free(true)
{

}

Serial::~Serial()
{
    uart = nullptr;
}

void Serial::ReadSerial()
{

}

void Serial::WriteSerial(const uint8_t* data, uint32_t len)
{
    if (!is_free)
    {
        Error_Handler();
        return;
    }

    is_free = false;
    HAL_UART_Transmit_DMA(uart, data, len);
}

bool Serial::IsFree()
{
    return is_free;
}

void Serial::Free()
{
    is_free = true;
}
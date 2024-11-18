#pragma once

#include "main.h"

class Serial
{
public:
    Serial(UART_HandleTypeDef* uart);
    ~Serial();

    void ReadSerial();
    void WriteSerial(const uint8_t* data, uint32_t len);

    bool IsFree();
    void Free();

protected:
    UART_HandleTypeDef* uart;
    bool is_free;
};
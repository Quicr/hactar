#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include "main.h"
#include "uart_router.h"

#define BAUD 115200

void PowerCycle(GPIO_TypeDef* port, uint16_t pin, uint32_t delay);
void HoldInReset(GPIO_TypeDef* port, uint16_t pin);
void ChangeToInput(GPIO_TypeDef* port, uint16_t pin);
void ChangeToOutput(GPIO_TypeDef* port, uint16_t pin);
void NormalAndNetUploadUartInit(uart_stream_t* stream, transmit_t* tx);
void UIUploadStreamInit(uart_stream_t* stream, transmit_t* tx);

#endif
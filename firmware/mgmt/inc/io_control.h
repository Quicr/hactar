#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include "main.h"
#include "uart_router.h"

#define BAUD 115200

void io_control_power_cycle(GPIO_TypeDef* port, uint16_t pin, uint32_t delay);
void io_control_change_to_input(GPIO_TypeDef* port, uint16_t pin);
void io_control_change_to_output(GPIO_TypeDef* port, uint16_t pin);

#endif
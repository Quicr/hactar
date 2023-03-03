#ifndef PORT_PIN_H
#define PORT_PIN_H

#include "stm32.h"

typedef struct port_pin_t
{
    GPIO_TypeDef *port;
    uint16_t pin;
} port_pin;

#endif
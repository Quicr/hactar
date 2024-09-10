#ifndef IO_CONTROL_H
#define IO_CONTROL_H

#include "main.h"

#define BAUD 115200


void PowerCycle(GPIO_TypeDef* port, uint16_t pin, uint32_t delay);
void ChangeToInput(GPIO_TypeDef* port, uint16_t pin);
void ChangeToOutput(GPIO_TypeDef* port, uint16_t pin);
void Usart1_Net_Upload_Runnning_Debug_Reset();
void Usart1_UI_Upload_Init();

#endif
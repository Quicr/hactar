#include "stm32f0xx_hal.h"
#include <string.h>

void UART_SendString(UART_HandleTypeDef* huart, const char* str)
{
    // Get the len
    uint16_t len = strlen(str);

    HAL_UART_Transmit(huart, str, len, HAL_MAX_DELAY);
}
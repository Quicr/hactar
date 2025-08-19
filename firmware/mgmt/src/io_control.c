#include "io_control.h"

void PowerCycle(GPIO_TypeDef* port, uint16_t pin, uint32_t delay)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    HAL_Delay(delay);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

/**
 * @brief TODO
 *
 */
void NormalAndNetUploadUartInit(uart_stream_t* usb_stream, transmit_t* tx)
{
    // Init uart1 for UI upload
    if (HAL_UART_DeInit(usb_stream->rx->uart) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->rx->uart->Init.WordLength = UART_WORDLENGTH_8B;
    usb_stream->rx->uart->Init.Parity = UART_PARITY_NONE;
    if (HAL_UART_Init(usb_stream->rx->uart) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->tx = tx;
}

/**
 * @brief TODO
 *
 */
void UIUploadStreamInit(uart_stream_t* usb_stream, transmit_t* tx)
{
    // Init uart1 for UI upload
    if (HAL_UART_DeInit(usb_stream->rx->uart) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->rx->uart->Init.WordLength = UART_WORDLENGTH_9B;
    usb_stream->rx->uart->Init.Parity = UART_PARITY_EVEN;
    if (HAL_UART_Init(usb_stream->rx->uart) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->tx = tx;
}

void ChangeToInput(GPIO_TypeDef* port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);

    HAL_GPIO_DeInit(port, pin);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void ChangeToOutput(GPIO_TypeDef* port, uint16_t pin)
{
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);

    HAL_GPIO_DeInit(port, pin);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}
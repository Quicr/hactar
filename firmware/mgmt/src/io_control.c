#include "io_control.h"

extern UART_HandleTypeDef huart1;

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
void NormalAndNetUploadUartInit(uart_stream_t* usb_stream, UART_HandleTypeDef* tx_uart)
{
    // Init uart1 for UI upload
    HAL_UART_DeInit(&huart1);

    huart1.Instance = USART1;
    huart1.Init.BaudRate = BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->tx.uart = tx_uart;
}

/**
 * @brief TODO
 *
 */
void UIUploadStreamInit(uart_stream_t* usb_stream, UART_HandleTypeDef* tx_uart)
{
    // Init uart1 for UI upload
    if (HAL_UART_DeInit(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    huart1.Instance = USART1;
    huart1.Init.BaudRate = BAUD;
    huart1.Init.WordLength = UART_WORDLENGTH_9B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_EVEN;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    usb_stream->tx.uart = tx_uart;
}
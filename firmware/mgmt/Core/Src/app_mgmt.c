/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "app_mgmt.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <string.h>
#include "stm32f0xx_hal_def.h"
#include "uart_stream.h"
/* USER CODE END Includes */

extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// TODO add messages that to the monitor

#define CPU_FREQ 48'000'000

#define BAUD 115200
#define UI_RECEIVE_BUFF_SZ 20
#define UI_TRANSMIT_BUFF_SZ UI_RECEIVE_BUFF_SZ * 2
#define NET_RECEIVE_BUFF_SZ 1024
#define NET_TRANSMIT_BUFF_SZ NET_RECEIVE_BUFF_SZ * 2
#define COMMAND_BUFF_SZ 16

#define BTN_PRESS_TIMEOUT 5000
#define BTN_DEBOUNCE_TIMEOUT 50
#define BTN_WAIT_TIMEOUT 1000

enum State state;
enum State next_state;

uart_stream_t usb_stream;
uart_stream_t net_stream;
uart_stream_t ui_stream;

uint32_t wait_timeout = 0;

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == USB_RTS_Pin)
    {
        if ((state == Net_Upload || state == UI_Upload) && usb_stream.idle_receive)
        {
            state = Reset;
        }
        return;
    }

    // Gets here then then it was a button that was pressed
    state = Waiting;
    wait_timeout = HAL_GetTick() + BTN_WAIT_TIMEOUT;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    // There are 3 conditions that call this function.

    // 1. Half complete -- The rx buffer is half full
    // 2. rx complete -- the rx buffer is full
    // 3. idle  -- Nothing has been received in awhile

    // Need to have as separate if statements so we can loop back properly
    // Which doesn't make any sense to me, but it makes it work.
    if (huart->Instance == net_stream.from_uart->Instance)
    {
        HandleRx(&net_stream, size);
    }

    if (huart->Instance == ui_stream.from_uart->Instance)
    {
        HandleRx(&ui_stream, size);
    }

    if (huart->Instance == usb_stream.from_uart->Instance)
    {
        HandleRx(&usb_stream, size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // Need to have as separate if statements so we can loop back properly
    // Which doesn't make any sense to me, but it makes it work.
    if (huart->Instance == net_stream.to_uart->Instance)
    {
        net_stream.tx_free = 1;
        net_stream.last_transmission_time = HAL_GetTick();
    }

    if (huart->Instance == ui_stream.to_uart->Instance)
    {
        ui_stream.tx_free = 1;
        ui_stream.last_transmission_time = HAL_GetTick();
    }

    if (huart->Instance == usb_stream.to_uart->Instance)
    {
        usb_stream.tx_free = 1;
        usb_stream.last_transmission_time = HAL_GetTick();
    }
}

void CancelAllUart()
{
    CancelUart(&ui_stream);
    CancelUart(&net_stream);
    CancelUart(&usb_stream);
}

void NetBootloaderMode()
{
    // Bring the boot low for esp, bootloader mode (0)
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_RESET);

    // Power cycle
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
}

void NetNormalMode()
{
    HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, GPIO_PIN_SET);

    // Power cycle
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_SET);
}

void NetHoldInReset()
{
    // Reset
    HAL_GPIO_WritePin(NET_RST_GPIO_Port, NET_RST_Pin, GPIO_PIN_RESET);
}

void UIBootloaderMode()
{
    // Normal boot mode (boot0 = 1 and boot1 = 0)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_RESET);

    // Power cycle
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
}

void UINormalMode()
{
    // Normal boot mode (boot0 = 0 and boot1 = 1)
    HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, GPIO_PIN_SET);

    // Power cycle
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_SET);
}

void UIHoldInReset()
{
    HAL_GPIO_WritePin(UI_RST_GPIO_Port, UI_RST_Pin, GPIO_PIN_RESET);
}

void NetUpload()
{
    NetHoldInReset();
    UIHoldInReset();

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);

    CancelAllUart();

    // Deinit usb
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&net_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&net_stream);

    // Put net into bootloader mode
    UIHoldInReset();
    NetBootloaderMode();

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    state = Net_Upload;

    HAL_Delay(500);

    // Send a ready message
    HAL_UART_Transmit(usb_stream.from_uart, READY, 1, HAL_MAX_DELAY);

    usb_stream.last_transmission_time = HAL_GetTick();
    net_stream.last_transmission_time = HAL_GetTick();


    while (state == Net_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&net_stream, &state);
    }
}

void UIUpload()
{
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_UI_Upload_Init();

    usb_stream.to_uart = &huart2;
    usb_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);

    NetHoldInReset();
    UIBootloaderMode();

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    state = UI_Upload;

    HAL_Delay(500);

    // Send a ready message
    HAL_UART_Transmit(usb_stream.from_uart, READY, 1, HAL_MAX_DELAY);

    usb_stream.last_transmission_time = HAL_GetTick();
    ui_stream.last_transmission_time = HAL_GetTick();

    while (state == UI_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&ui_stream, &state);
    }
}

void RunningMode()
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_RESET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_RESET);

    CancelAllUart();

    // De-init usb from uart for running mode
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = COMMAND_BUFF_SZ;
    usb_stream.tx_buffer_size = COMMAND_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    StartUartReceive(&usb_stream);

    UINormalMode();
    uint32_t timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the UI is finished booting
        HAL_Delay(10);
    }

    NetNormalMode();
    // Refresh the timeout
    timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_NET_STAT_GPIO_Port, ADC_NET_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the Net is done booting
        HAL_Delay(10);
    }

    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    state = Running;
    while (state == Running)
    {
        HandleCommands(&usb_stream, &huart1, &state);
    }
}

void DebugMode()
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.from_uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    // usb_stream.to_uart = &huart3;
    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;
    ui_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    ui_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;
    net_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    net_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);
    InitUartStreamParameters(&net_stream);
    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);
    StartUartReceive(&net_stream);

    HAL_Delay(100);

    UINormalMode();

    uint32_t timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the UI is finished booting
        HAL_Delay(10);
    }

    NetNormalMode();
    // Refresh the timeout
    timeout = HAL_GetTick() + 3000;
    while (HAL_GetTick() < timeout &&
        HAL_GPIO_ReadPin(ADC_NET_STAT_GPIO_Port, ADC_NET_STAT_Pin) != GPIO_PIN_SET)
    {
        // Stay here until the Net is done booting
        HAL_Delay(10);
    }

    state = Debug_Running;
    while (state == Debug_Running)
    {
        HandleCommands(&usb_stream, &huart1, &state);
        HandleTx(&ui_stream, &state);
        HandleTx(&net_stream, &state);
    }
}

int app_main(void)
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, GPIO_PIN_SET);

    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, GPIO_PIN_SET);

    // TODO move these into functions for starting ui/net upload
    // Init uart structures

    usb_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    usb_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    usb_stream.rx_buffer = (uint8_t*)malloc(usb_stream.rx_buffer_size * sizeof(uint8_t));
    usb_stream.tx_buffer = (uint8_t*)malloc(usb_stream.tx_buffer_size * sizeof(uint8_t));

    net_stream.rx_buffer_size = NET_RECEIVE_BUFF_SZ;
    net_stream.tx_buffer_size = NET_TRANSMIT_BUFF_SZ;

    net_stream.rx_buffer = (uint8_t*)malloc(net_stream.rx_buffer_size * sizeof(uint8_t));
    net_stream.tx_buffer = (uint8_t*)malloc(net_stream.tx_buffer_size * sizeof(uint8_t));

    ui_stream.rx_buffer_size = UI_RECEIVE_BUFF_SZ;
    ui_stream.tx_buffer_size = UI_TRANSMIT_BUFF_SZ;

    ui_stream.rx_buffer = (uint8_t*)malloc(ui_stream.rx_buffer_size * sizeof(uint8_t));
    ui_stream.tx_buffer = (uint8_t*)malloc(ui_stream.tx_buffer_size * sizeof(uint8_t));

    usb_stream.from_uart = &huart1;
    usb_stream.to_uart = &huart1;
    InitUartStreamParameters(&usb_stream);

    net_stream.from_uart = &huart3;
    net_stream.to_uart = &huart1;
    InitUartStreamParameters(&net_stream);

    ui_stream.from_uart = &huart2;
    ui_stream.to_uart = &huart1;
    InitUartStreamParameters(&ui_stream);

    // NOTE these need to be set to 1 so when we choose a mode
    // It can properly de-init and re-init the huart modes
    // Its weird but hey, its what works with HAL
    usb_stream.is_listening = 1;
    net_stream.is_listening = 1;
    ui_stream.is_listening = 1;

    state = Waiting;
    next_state = Reset;
    while (1)
    {
        if (state == Waiting)
        {
            state = next_state;
            next_state = Waiting;
            while (HAL_GetTick() < wait_timeout)
            {
                __NOP();
            }
        }
        else if (state == Reset)
        {
            RunningMode();
        }
        else if (state == Debug_Reset)
        {
            DebugMode();
        }
        else if (state == UI_Upload_Reset)
        {
            UIUpload();
        }
        else if (state == Net_Upload_Reset)
        {
            NetUpload();
        }
    }
    return 0;
}

void Usart1_Net_Upload_Runnning_Debug_Reset(void)
{
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

}

void Usart1_UI_Upload_Init(void)
{
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

}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
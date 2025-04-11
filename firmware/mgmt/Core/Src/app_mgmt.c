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
#include "chip_control.h"
#include "io_control.h"
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

// TODO make a proper ring buffer for writing into the
// different rings that will then be self managed instead of
// each one trying to fight to write to dma

#define CPU_FREQ 48'000'000

#define UART_BUFF_SZ 1024
#define COMMAND_BUFF_SZ 32
#define TRANSMISSION_TIMEOUT 10000

uint8_t ui_rx_buff[UART_BUFF_SZ] = { 0 };
uint8_t ui_tx_buff[UART_BUFF_SZ] = { 0 };

uint8_t net_rx_buff[UART_BUFF_SZ] = { 0 };
uint8_t net_tx_buff[UART_BUFF_SZ] = { 0 };

uint8_t usb_rx_buff[UART_BUFF_SZ] = { 0 };
uint8_t usb_tx_buff[UART_BUFF_SZ] = { 0 };

uint8_t command_buff[COMMAND_BUFF_SZ] = { 0 };

uint32_t timeout_tick = 0;

enum State state = Reset;

uart_stream_t ui_stream = {
    .rx = {
        .uart = &huart2,
        .buff = ui_rx_buff,
        .size = UART_BUFF_SZ,
        .idx = 0
    },
    .tx = {
        .uart = &huart1,
        .buff = ui_tx_buff,
        .size = UART_BUFF_SZ,
        .read = 0,
        .write = 0,
        .unsent = 0,
        .free = 1
    },
    .mode = Ignore,
};

uart_stream_t net_stream = {
    .rx = {
        .uart = &huart3,
        .buff = net_rx_buff,
        .size = UART_BUFF_SZ,
        .idx = 0
    },
    .tx = {
        .uart = &huart1,
        .buff = net_tx_buff,
        .size = UART_BUFF_SZ,
        .read = 0,
        .write = 0,
        .unsent = 0,
        .free = 1
    },
    .mode = Ignore,
};

uart_stream_t usb_stream = {
    .rx = {
        .uart = &huart1,
        .buff = usb_rx_buff,
        .size = UART_BUFF_SZ,
        .idx = 0
    },
    .tx = {
        .uart = &huart1,
        .buff = usb_tx_buff,
        .size = UART_BUFF_SZ,
        .read = 0,
        .write = 0,
        .unsent = 0,
        .free = 1
    },
    .mode = Ignore,
};

cmd_ring_buff_t cmd_ring = {
    .buff = command_buff,
    .sz = COMMAND_BUFF_SZ,
    .idx = 0,
    .last_update = 0,
};

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == UI_STAT_Pin)
    {
        if (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_SET)
        {
            HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(NET_STAT_GPIO_Port, NET_STAT_Pin, GPIO_PIN_SET);
        }
        else if (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_RESET)
        {
            HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(NET_STAT_GPIO_Port, NET_STAT_Pin, GPIO_PIN_RESET);
        }
    }

}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t rx_idx)
{
    // There are 3 conditions that call this function.

    // 1. Half complete -- The rx buffer is half full
    // 2. rx complete -- The rx buffer is full
    // 3. idle  -- Nothing has been received in awhile

    // Need to have as separate if statements so we can loop back properly
    // Which doesn't make any sense to me, but it makes it work.

    HAL_GPIO_TogglePin(LEDB_G_GPIO_Port, LEDB_G_Pin);
    if (huart->Instance == net_stream.rx.uart->Instance)
    {
        Receive(&net_stream, rx_idx);
        return;
    }

    if (huart->Instance == ui_stream.rx.uart->Instance)
    {
        Receive(&ui_stream, rx_idx);
        return;
    }

    if (huart->Instance == usb_stream.rx.uart->Instance)
    {
        timeout_tick = HAL_GetTick();
        Receive(&usb_stream, rx_idx);
        return;
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // Since net_stream.tx.uart is usb AND ui_stream.tx.uart is usb
    // then when this is called during either ui upload or net upload
    // then they both need to be notified that the usb is free. :shrug:
    HAL_GPIO_TogglePin(LEDB_B_GPIO_Port, LEDB_B_Pin);
    if (huart->Instance == net_stream.tx.uart->Instance)
    {
        Transmit(&net_stream, &state);
    }

    if (huart->Instance == ui_stream.tx.uart->Instance)
    {
        Transmit(&ui_stream, &state);
    }

    if (huart->Instance == usb_stream.tx.uart->Instance)
    {
        Transmit(&usb_stream, &state);
    }
}

void CancelAllUart()
{
    CancelUart(&ui_stream);
    CancelUart(&net_stream);
    CancelUart(&usb_stream);
}

void NetUpload()
{
    TurnOffLEDs();

    NetHoldInReset();
    UIHoldInReset();

    CancelAllUart();

    // Deinit usb
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart3;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&net_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&net_stream);

    // Put net into bootloader mode
    UIHoldInReset();
    NetBootloaderMode();

    // Set LEDS for ui
    LEDB(HIGH, HIGH, HIGH);

    state = Net_Upload;

    HAL_Delay(500);

    usb_stream.mode = Passthrough;
    net_stream.mode = Passthrough;

    // Send a ready message
    HAL_UART_Transmit(usb_stream.rx.uart, READY, 1, HAL_MAX_DELAY);

    timeout_tick = HAL_GetTick();
    while (state == Net_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&net_stream, &state);
        CheckTimeout();
    }
}

void UIUpload()
{
    TurnOffLEDs();

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_UI_Upload_Init();

    usb_stream.tx.uart = &huart2;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);

    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);

    NetHoldInReset();
    UIBootloaderMode();

    // Set LEDS for ui
    LEDB(HIGH, LOW, HIGH);

    // Set LEDS for net
    LEDA(HIGH, HIGH, HIGH);

    state = UI_Upload;

    usb_stream.mode = Passthrough;
    ui_stream.mode = Passthrough;

    HAL_Delay(500);

    // Send a ready message
    HAL_UART_Transmit(usb_stream.rx.uart, READY, 1, HAL_MAX_DELAY);

    timeout_tick = HAL_GetTick();
    while (state == UI_Upload)
    {
        HandleTx(&usb_stream, &state);
        HandleTx(&ui_stream, &state);
        CheckTimeout();
    }
}

void RunningMode()
{
    TurnOffLEDs();

    CancelAllUart();

    // De-init usb from uart for running mode
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart1;

    InitUartStreamParameters(&usb_stream);
    StartUartReceive(&usb_stream);

    NormalStart();

    // Set LEDS for ui
    LEDB(LOW, HIGH, HIGH);
    // Set LEDS for net
    LEDA(HIGH, HIGH, HIGH);

    state = Running;
    WaitForNetReady(&state);
    while (state == Running)
    {
        HandleCommands(&usb_stream, &cmd_ring, &state);
    }
}

void DebugMode()
{
    TurnOffLEDs();

    CancelAllUart();

    // Init uart1 to write to monitor
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart1;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);
    InitUartStreamParameters(&net_stream);
    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);
    StartUartReceive(&net_stream);

    usb_stream.mode = Passthrough;
    ui_stream.mode = Passthrough;
    net_stream.mode = Passthrough;

    HAL_Delay(100);

    NormalStart();

    // Set LEDS for ui
    LEDB(HIGH, LOW, HIGH);
    // Set LEDS for net
    LEDA(LOW, LOW, LOW);

    state = Debug_Running;
    WaitForNetReady(&state);
    while (state == Debug_Running)
    {
        HandleCommands(&usb_stream, &cmd_ring, &state);
        HandleTx(&ui_stream, &state);
        HandleTx(&net_stream, &state);
    }
}
void UIDebugMode()
{
    TurnOffLEDs();

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart1;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&ui_stream);
    StartUartReceive(&usb_stream);
    StartUartReceive(&ui_stream);

    HAL_Delay(100);
    NormalStart();

    // Set LEDS for ui
    LEDB(HIGH, HIGH, HIGH);
    // Set LEDS for net
    LEDA(HIGH, LOW, HIGH);

    state = Debug_Running;
    WaitForNetReady(&state);
    while (state == Debug_Running)
    {
        HandleCommands(&usb_stream, &cmd_ring, &state);
        HandleTx(&ui_stream, &state);
    }
}

void NetDebugMode()
{
    TurnOffLEDs();

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart1;

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&net_stream);
    StartUartReceive(&usb_stream);
    StartUartReceive(&net_stream);

    HAL_Delay(100);

    NormalStart();

    // Set LEDS for ui
    LEDB(HIGH, HIGH, HIGH);

    // Set LEDS for net
    LEDA(HIGH, HIGH, LOW);

    state = Debug_Running;
    WaitForNetReady(&state);
    while (state == Debug_Running)
    {
        HandleCommands(&usb_stream, &cmd_ring, &state);
        HandleTx(&net_stream, &state);
    }
}

void LoopbackMode()
{
    TurnOffLEDs();

    CancelAllUart();

    // Init uart3 for UI upload
    HAL_UART_DeInit(usb_stream.rx.uart);

    // Init huart3
    Usart1_Net_Upload_Runnning_Debug_Reset();

    usb_stream.tx.uart = &huart1;

    InitUartStreamParameters(&usb_stream);

    StartUartReceive(&usb_stream);

    NetHoldInReset();
    UIHoldInReset();

    // Set LEDS for ui
    LEDB(LOW, HIGH, HIGH);

    // Set LEDS for net
    LEDA(HIGH, HIGH, HIGH);

    usb_stream.mode = Passthrough;


    uint32_t blink = 0;
    while (state == 6)
    {
        if (HAL_GetTick() > blink)
        {
            HAL_GPIO_TogglePin(LEDA_R_GPIO_Port, LEDA_R_Pin);
            blink = HAL_GetTick() + 1000;
        }
        HandleTx(&usb_stream, &state);
        CheckTimeout();
    }
}

int app_main(void)
{
    TurnOffLEDs();

    InitUartStreamParameters(&usb_stream);
    InitUartStreamParameters(&net_stream);
    InitUartStreamParameters(&ui_stream);

    state = Reset;
    while (1)
    {
        if (state == Reset)
        {
            RunningMode();
        }
        else if (state == Debug_Reset)
        {
            DebugMode();
        }
        else if (state == UI_Debug_Reset)
        {
            UIDebugMode();
        }
        else if (state == Net_Debug_Reset)
        {
            NetDebugMode();
        }
        else if (state == UI_Upload_Reset)
        {
            UIUpload();
        }
        else if (state == Net_Upload_Reset)
        {
            NetUpload();
        }
        else if (state == 6)
        {
            LoopbackMode();
        }
    }
    return 0;
}

void CheckTimeout()
{
    if (HAL_GetTick() - timeout_tick >= TRANSMISSION_TIMEOUT &&
        state != Debug_Running &&
        state != Reset &&
        state != Running)
    {
        // Clean up and return to reset mode
        state = Reset;
        return;
    }
}

void TurnOffLEDs()
{
    LEDA(HIGH, HIGH, HIGH);
    LEDB(HIGH, HIGH, HIGH);
}

/**
 * @brief TODO
 *
 */
void LEDA(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b)
{
    // Set LEDS for net
    HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, r);
    HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, g);
    HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, b);
}

/**
 * @brief TODO
 *
 */
void LEDB(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b)
{
    // Set LEDS for ui
    HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, r);
    HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, g);
    HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, b);
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
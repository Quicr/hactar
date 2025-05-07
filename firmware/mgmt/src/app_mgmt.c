/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "app_mgmt.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "chip_control.h"
#include "io_control.h"
#include "stm32f0xx_hal_def.h"
#include "uart_stream.h"
#include <stdlib.h>
#include <string.h>
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
#define TRANSMISSION_TIMEOUT 10000

uint8_t ui_rx_buff[UART_BUFF_SZ] = {0};
uint8_t ui_tx_buff[UART_BUFF_SZ] = {0};

uint8_t net_rx_buff[UART_BUFF_SZ] = {0};
uint8_t net_tx_buff[UART_BUFF_SZ] = {0};

uint8_t usb_rx_buff[UART_BUFF_SZ] = {0};
uint8_t usb_tx_buff[UART_BUFF_SZ] = {0};

static uint8_t uploader = 0;
static uint32_t timeout_tick = 0;

const enum State default_state = Debug;
enum State state = default_state;

uart_stream_t ui_stream = {
    .rx = {.uart = &huart2, .buff = ui_rx_buff, .size = UART_BUFF_SZ, .idx = 0},
    .tx = { .uart = &huart1,
           .buff = ui_tx_buff,
           .size = UART_BUFF_SZ,
           .read = 0,
           .write = 0,
           .unsent = 0,
           .num_sending = 0,
           .free = 1},
    .mode = Ignore,
};

uart_stream_t net_stream = {
    .rx = {.uart = &huart3, .buff = net_rx_buff, .size = UART_BUFF_SZ, .idx = 0},
    .tx = { .uart = &huart1,
           .buff = net_tx_buff,
           .size = UART_BUFF_SZ,
           .read = 0,
           .write = 0,
           .unsent = 0,
           .num_sending = 0,
           .free = 1},
    .mode = Ignore,
};

uart_stream_t usb_stream = {
    .rx = {.uart = &huart1, .buff = usb_rx_buff, .size = UART_BUFF_SZ, .idx = 0},
    .tx = { .uart = &huart1,
           .buff = usb_tx_buff,
           .size = UART_BUFF_SZ,
           .read = 0,
           .write = 0,
           .unsent = 0,
           .num_sending = 0,
           .free = 1},
    .mode = Ignore,
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
    __disable_irq();
    if (huart->Instance == net_stream.rx.uart->Instance)
    {
        Receive(&net_stream, rx_idx);
    }
    else if (huart->Instance == ui_stream.rx.uart->Instance)
    {
        Receive(&ui_stream, rx_idx);
    }
    else if (huart->Instance == usb_stream.rx.uart->Instance)
    {
        timeout_tick = HAL_GetTick();
        Receive(&usb_stream, rx_idx);
    }
    __enable_irq();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // Since net_stream.tx.uart is usb AND ui_stream.tx.uart is usb
    // then when this is called during either ui upload or net upload
    // then they both need to be notified that the usb is free. :shrug:
    HAL_GPIO_TogglePin(LEDB_B_GPIO_Port, LEDB_B_Pin);
    __disable_irq();
    if (!net_stream.tx.free && huart->Instance == net_stream.tx.uart->Instance)
    {
        TxISR(&net_stream, &state);
    }
    else if (!ui_stream.tx.free && huart->Instance == ui_stream.tx.uart->Instance)
    {
        TxISR(&ui_stream, &state);
    }
    else if (!usb_stream.tx.free && huart->Instance == usb_stream.tx.uart->Instance)
    {
        TxISR(&usb_stream, &state);
    }
    __enable_irq();
}

void CancelAllUart()
{
    HAL_UART_Abort(usb_stream.rx.uart);
    HAL_UART_Abort(ui_stream.rx.uart);
    HAL_UART_Abort(net_stream.rx.uart);
}

int app_main(void)
{
    state = default_state;

    while (1)
    {
        uploader = 0;
        TurnOffLEDs();
        CancelAllUart();

        NormalAndNetUploadUartInit(&usb_stream, &huart1);

        switch (state)
        {
        case Error:
        {
            Error_Handler();
            break;
        }
        case Running:
        {
            // If we get here its an error
            // because there should never be anyway it gets out of the main loop with this state
            Error_Handler();
            break;
        }
        case UI_Upload:
        {
            NetHoldInReset();

            UIUploadStreamInit(&usb_stream, &huart2);
            SetStreamModes(Passthrough, Passthrough, Ignore);
            UIBootloaderMode();
            uploader = 1;
            LEDA(HIGH, HIGH, LOW);

            break;
        }
        case Net_Upload:
        {
            UIHoldInReset();
            NormalAndNetUploadUartInit(&usb_stream, &huart3);
            SetStreamModes(Passthrough, Ignore, Passthrough);
            NetBootloaderMode();
            uploader = 1;
            LEDA(HIGH, LOW, HIGH);
            break;
        }
        case Normal:
        {
            SetStreamModes(Command, Ignore, Ignore);
            NormalBoot();
            LEDA(HIGH, LOW, LOW);
            break;
        }
        case Debug:
        {
            SetStreamModes(Command, Passthrough, Passthrough);
            NormalBoot();
            LEDA(LOW, HIGH, HIGH);
            break;
        }
        case UI_Debug:
        {
            SetStreamModes(Command, Passthrough, Ignore);
            NormalBoot();
            LEDA(LOW, HIGH, LOW);
            break;
        }
        case Net_Debug:
        {
            SetStreamModes(Command, Ignore, Passthrough);
            NormalBoot();
            LEDA(LOW, LOW, HIGH);
            break;
        }
        case Loopback:
        {
            SetStreamModes(Passthrough, Ignore, Ignore);
            NormalBoot();
            LEDA(LOW, LOW, LOW);
            break;
        }
        default:
        {
            Error_Handler();
        }
        }

        state = Running;
        while (state == Running)
        {
            HandleTx(&ui_stream, &state);
            HandleTx(&net_stream, &state);
            HandleTx(&usb_stream, &state);
            CheckTimeout();
        }
    }
    return 0;
}

void NormalBoot()
{
    UINormalMode();
    NetNormalMode();
}

void CheckTimeout()
{
    if (uploader && HAL_GetTick() - timeout_tick >= TRANSMISSION_TIMEOUT)
    {
        // Clean up and return to reset mode
        state = default_state;
        return;
    }
}

void SetStreamModes(const StreamMode usb_mode, const StreamMode ui_mode, const StreamMode net_mode)
{
    usb_stream.mode = usb_mode;
    if (usb_stream.mode != Ignore)
    {
        InitUartStream(&usb_stream);
        StartUartReceive(&usb_stream);
    }

    ui_stream.mode = ui_mode;
    if (ui_stream.mode != Ignore)
    {
        InitUartStream(&ui_stream);
        StartUartReceive(&ui_stream);
    }

    net_stream.mode = net_mode;
    if (net_stream.mode != Ignore)
    {
        InitUartStream(&net_stream);
        StartUartReceive(&net_stream);
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

#ifdef USE_FULL_ASSERT
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
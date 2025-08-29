#include "app_mgmt.h"
#include "chip_control.h"
#include "io_control.h"
#include "main.h"
#include "stm32f0xx_hal_def.h"
#include "uart_router.h"
#include "uart_stream.h"
#include <stdlib.h>
#include <string.h>

extern TIM_HandleTypeDef htim3;

// TODO add messages that to the monitor

#define CPU_FREQ 48'000'000

#define TRANSMISSION_TIMEOUT 10000

static uint8_t uploader = 0;
static uint32_t timeout_tick = 0;

enum State state = default_state;
enum State next_state = Running;

const command_map_t command_map[Cmd_Count] = {
    {Cmd_Version, command_get_version, NULL},
    {Cmd_Who_Are_You, command_who_are_you, NULL},
    {Cmd_Hard_Reset, command_hard_reset, NULL},
    {Cmd_Reset, command_reset, NULL},
    {Cmd_Reset_Ui, command_reset_ui, NULL},
    {Cmd_Reset_Net, command_reset_net, NULL},
    {Cmd_Flash_Ui, command_flash_ui, (void*)&uploader},
};

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    __disable_irq();
    // if (huart->Instance == huart1.Instance)
    // {
    //     if (usb_stream.mode != Ignore)
    //     {
    //         InitUartStream(&usb_stream);
    //         StartUartReceive(&usb_stream);
    //     }
    // }
    // else if (huart->Instance == huart2.Instance)
    // {
    //     if (ui_stream.mode != Ignore)
    //     {
    //         InitUartStream(&ui_stream);
    //         StartUartReceive(&ui_stream);
    //     }
    // }
    __enable_irq();
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == UI_STAT_Pin)
    {
        if (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_SET)
        {
            // HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(NET_STAT_GPIO_Port, NET_STAT_Pin, GPIO_PIN_SET);
        }
        else if (HAL_GPIO_ReadPin(UI_STAT_GPIO_Port, UI_STAT_Pin) == GPIO_PIN_RESET)
        {
            // HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(NET_STAT_GPIO_Port, NET_STAT_Pin, GPIO_PIN_RESET);
        }
    }
}

void StartUartReceive(uart_stream_t* uart_stream)
{
    uint8_t attempt = 0;
    while (attempt++ != 10
           && HAL_OK
                  != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->rx->uart, uart_stream->rx->buff,
                                                  uart_stream->rx->size))
    {
        // Make sure the uart is cancelled, sometimes it doesn't want to cancel
        HAL_UART_Abort(uart_stream->rx->uart);
    }

    if (attempt >= 10)
    {
        Error_Handler();
    }
}

int app_main(void)
{
    state = default_state;

    uart_stream_t* usb_stream = uart_router_get_usb_stream();
    uart_stream_t* net_stream = uart_router_get_net_stream();
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    StartUartReceive(usb_stream);
    usb_stream->path = Tx_Path_Internal;
    while (1)
    {
        NetHoldInReset();
        UIHoldInReset();
        uploader = 0;
        next_state = Running;
        TurnOffLEDs();
        CancelAllUart();

        // InitUartStream(usb_stream);
        // StartUartReceive(ui_stream);
        // ui_stream->path = Tx_Path_Usb;

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

            // UIUploadStreamInit(&usb_stream, &ui_tx);
            SendUploadOk();

            // SetStreamModes(Passthrough, Passthrough, Ignore);

            UIBootloaderMode();
            uploader = 1;
            // HAL_UART_Transmit(usb_stream.rx->uart, Ready_Byte, 1, HAL_MAX_DELAY);

            LEDA(HIGH, HIGH, LOW);

            break;
        }
        case Net_Upload:
        {
            UIHoldInReset();

            // NormalAndNetUploadUartInit(&usb_stream, &net_tx);
            SendUploadOk();

            // SetStreamModes(Passthrough, Ignore, Passthrough);

            NetBootloaderMode();
            uploader = 1;
            // HAL_UART_Transmit(usb_stream.rx->uart, Ready_Byte, 1, HAL_MAX_DELAY);

            LEDA(HIGH, LOW, HIGH);

            break;
        }
        case Normal:
        {
            NormalInit();
            // SetStreamModes(Command, Ignore, Ignore);
            LEDA(HIGH, LOW, LOW);
            break;
        }
        case Debug:
        {
            NormalInit();
            // SetStreamModes(Command, Passthrough, Passthrough);
            LEDA(LOW, HIGH, HIGH);
            break;
        }
        case UI_Debug:
        {
            NormalInit();
            // SetStreamModes(Command, Passthrough, Ignore);
            LEDA(LOW, HIGH, LOW);
            break;
        }
        case Net_Debug:
        {
            NormalInit();
            // SetStreamModes(Command, Ignore, Passthrough);
            LEDA(LOW, LOW, HIGH);
            break;
        }
        case Loopback:
        {
            NormalInit();
            // SetStreamModes(Passthrough, Ignore, Ignore);
            LEDA(LOW, LOW, LOW);
            break;
        }
        case Configator:
        {
            UINormalMode();
            // NormalAndNetUploadUartInit(&usb_stream, &huart2);
            // SetStreamModes(Configuration, Passthrough, Ignore);
            LEDA(LOW, LOW, LOW);
            LEDB(LOW, LOW, LOW);
            break;
        }
        default:
        {
            Error_Handler();
        }
        }

        state = next_state;
        while (state == Running)
        {
            uart_router_transmit(usb_stream->tx);
            uart_router_transmit(ui_stream->tx);
            uart_router_transmit(net_stream->tx);
            uart_router_parse_internal(command_map);

            CheckTimeout();

            if (state != next_state)
            {
                state = next_state;
            }
        }
    }
    return 0;
}

void NormalInit()
{
    NetNormalMode();
    UINormalMode();
    // NormalAndNetUploadUartInit(&usb_stream, &usb_tx);
}

void CheckTimeout()
{
    if (uploader && HAL_GetTick() - timeout_tick >= TRANSMISSION_TIMEOUT)
    {
        // Clean up and return to reset mode
        next_state = default_state;
        return;
    }
}

void SendUploadOk()
{
    // static uint8_t rx_buff[1];
    // rx_buff[0] = 0;

    // for (int i = 0; i < 5; i++)
    // {
    //     // HAL_UART_Transmit(usb_stream.rx->uart, Ok_Byte, 1, HAL_MAX_DELAY);
    //     HAL_Delay(100);
    //     // HAL_UART_Receive(usb_stream.rx->uart, rx_buff, 1, 1000);

    //     if (rx_buff[0] == Ok_Byte[0])
    //     {
    //         return;
    //     }
    // }

    // // If we got here then the flasher never responded so put the state back to normal
    // next_state = default_state;
}

void CancelAllUart()
{
    // HAL_UART_Abort(usb_stream.rx->uart);
    // HAL_UART_Abort(ui_stream.rx->uart);
    // HAL_UART_Abort(net_stream.rx->uart);
}

// void SetStreamModes(const StreamMode usb_mode, const StreamMode ui_mode, const StreamMode
// net_mode)
// {
//     usb_stream.mode = usb_mode;
//     if (usb_stream.mode != Ignore)
//     {
//         InitUartStream(&usb_stream);
//         StartUartReceive(&usb_stream);
//     }

//     ui_stream.mode = ui_mode;
//     if (ui_stream.mode != Ignore)
//     {
//         InitUartStream(&ui_stream);
//         StartUartReceive(&ui_stream);
//     }

//     net_stream.mode = net_mode;
//     if (net_stream.mode != Ignore)
//     {
//         InitUartStream(&net_stream);
//         StartUartReceive(&net_stream);
//     }
// }

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

void app_mgmt_reset(const Reset_Type reset_type)
{
    if (reset_type == Hard_Reset)
    {
        next_state = default_state;
    }
}
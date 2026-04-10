#include "app_mgmt.h"
#include "chip_control.h"
#include "command_handler.h"
#include "io_control.h"
#include "main.h"
#include "stm32f0xx_hal_def.h"
#include "uart_router.h"
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim3;

// TODO add messages that to the monitor

#define CPU_FREQ 48'000'000

#define TRANSMISSION_TIMEOUT 10000

static uint8_t uploader = 0;

State state = default_state;
State next_state = Running;

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

int app_main(void)
{
    state = default_state;

    uart_stream_t* usb_stream = uart_router_get_usb_stream();
    uart_stream_t* net_stream = uart_router_get_net_stream();
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    while (1)
    {
        chip_control_net_normal_mode();
        chip_control_ui_normal_mode();

        usb_stream->path = Tx_Path_Internal;

        command_default_logging((void*)state);

        // uart_router_usb_update_reinit(UART_WORDLENGTH_8B, UART_PARITY_NONE);
        uart_router_start_receive(usb_stream);
        uart_router_start_receive(net_stream);
        uart_router_start_receive(ui_stream);

        TurnOffLEDs();

        uploader = 0;
        next_state = Running;
        state = next_state;
        while (state == Running)
        {

            uart_router_read_usb();
            uart_router_read_ui();
            uart_router_read_net();

            uart_router_transmit(usb_stream);
            uart_router_transmit(ui_stream);
            uart_router_transmit(net_stream);

            // CheckTimeout();
            //
            // if (state != next_state)
            // {
            //     state = next_state;
            // }
        }
    }
    return 0;
}

void CheckTimeout()
{
    if (!uploader)
    {
        return;
    }

    if (HAL_GetTick() - last_receive_tick < TRANSMISSION_TIMEOUT)
    {
        return;
    }

    // Clean up and return to reset mode
    next_state = default_state;
}

void TurnOffLEDs()
{
    LEDA(LOW, HIGH, HIGH);
}

/**
 * @brief TODO
 *
 */
void LEDA(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b)
{
    // Set LEDS for net
    // HAL_GPIO_WritePin(LEDA_R_GPIO_Port, LEDA_R_Pin, r);
    // HAL_GPIO_WritePin(LEDA_G_GPIO_Port, LEDA_G_Pin, g);
    // HAL_GPIO_WritePin(LEDA_B_GPIO_Port, LEDA_B_Pin, b);
}

/**
 * @brief TODO
 *
 */
void LEDB(GPIO_PinState r, GPIO_PinState g, GPIO_PinState b)
{
    // Set LEDS for ui
    // HAL_GPIO_WritePin(LEDB_R_GPIO_Port, LEDB_R_Pin, r);
    // HAL_GPIO_WritePin(LEDB_G_GPIO_Port, LEDB_G_Pin, g);
    // HAL_GPIO_WritePin(LEDB_B_GPIO_Port, LEDB_B_Pin, b);
}

void app_mgmt_reset(const Reset_Type reset_type)
{
    if (reset_type == Hard_Reset)
    {
        next_state = default_state;
    }
}

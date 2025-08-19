#ifndef UART_ROUTER_H
#define UART_ROUTER_H

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"

#define UART_BUFF_SZ 1024
#define COMMAND_TIMEOUT 1000
#define INTERNAL_BUFF_SZ 32
#define CONFIGURATOR_BUFF_SZ 128

typedef enum
{
    Ignore,
    Passthrough,
    Command,
    Configuration
} StreamMode;

typedef enum
{
    None = 0,
    Usb,
    Ui,
    Net,
    Ui_Net,
    Internal,
} Buffer_Direction;

typedef struct
{
    UART_HandleTypeDef* uart;
    uint8_t* buff;
    const uint16_t size;
    uint16_t idx;
} receive_t;

typedef struct
{
    UART_HandleTypeDef* uart;
    uint8_t* buff;
    const uint16_t size;
    uint16_t read;
    uint16_t write;
    uint16_t unsent;
    uint16_t num_sending;
    uint8_t free;
} transmit_t;

typedef struct
{
    receive_t* rx;
    transmit_t* tx;
    StreamMode mode;
    Buffer_Direction direction;
} uart_stream_t;

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received);
void uart_router_copy_rx_data(receive_t* rx, transmit_t* tx, const uint16_t num_bytes);
void uart_router_tx_isr(transmit_t* tx);
void uart_router_transmit(transmit_t* tx);
uart_stream_t* uart_router_get_ui_stream();
uart_stream_t* uart_router_get_net_stream();
uart_stream_t* uart_router_get_usb_stream();
uint32_t uart_router_get_last_received_tick();

#endif
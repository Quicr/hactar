#ifndef UART_ROUTER_H
#define UART_ROUTER_H

#include "command_handler.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"

#define UART_BUFF_SZ 1024
#define USB_UART_BUFF_SZ 4096
#define COMMAND_TIMEOUT 1000
#define INTERNAL_BUFF_SZ 64
#define PACKET_SZ 64

typedef enum
{
    Tx_Path_None = 0,
    Tx_Path_Usb,
    Tx_Path_Ui,
    Tx_Path_Net,
    Tx_Path_Ui_Net,
    Tx_Path_Internal,
} Tx_Path;

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
    Tx_Path path;
} uart_stream_t;

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received);
void uart_router_copy_to_tx(transmit_t* tx, const uint8_t* buff, const uint16_t num_bytes);
void uart_router_copy_string_to_tx(transmit_t* tx, const char* str);
void uart_router_tx_isr(transmit_t* tx);
void uart_router_transmit(transmit_t* tx);
void uart_router_perform_transmit(transmit_t* tx);
void uart_router_parse_internal(const command_map_t command_map[Cmd_Count]);
uart_stream_t* uart_router_get_ui_stream();
uart_stream_t* uart_router_get_net_stream();
uart_stream_t* uart_router_get_usb_stream();
uint32_t uart_router_get_last_received_tick();
void uart_router_update_last_received_tick(const uint32_t current_tick);
void uart_router_send_flash_ok();
void uart_router_send_ready();
void uart_router_usb_reinit(const uint32_t HAL_word_length, const uint32_t HAL_parity);
void uart_router_reset_stream(uart_stream_t* stream);

#endif
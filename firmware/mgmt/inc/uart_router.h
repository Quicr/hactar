#ifndef UART_ROUTER_H
#define UART_ROUTER_H

#include "command_handler.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"

#define NET_UART_BUFF_SZ 2048
#define USB_UART_BUFF_SZ 2048
#define UI_UART_BUFF_SZ 1024
#define INTERNAL_BUFF_SZ 64
#define PACKET_SZ 64
#define COMMAND_TIMEOUT 1000

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
    uint8_t* buff;
    const uint16_t size;
    uint16_t idx;
} receive_t;

typedef struct
{
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
    UART_HandleTypeDef* uart;
    receive_t rx;
    transmit_t tx;
    Tx_Path path;
} uart_stream_t;

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received);
void uart_router_copy_to_tx(transmit_t* tx, const uint8_t* buff, const uint16_t num_bytes);
void uart_router_copy_string_to_tx(transmit_t* tx, const char* str);
void uart_router_tx_isr(uart_stream_t* tx);
void uart_router_transmit(uart_stream_t* tx);
void uart_router_perform_transmit(uart_stream_t* tx);
void uart_router_parse_internal(const command_map_t command_map[Cmd_Count]);
uart_stream_t* uart_router_get_ui_stream();
uart_stream_t* uart_router_get_net_stream();
uart_stream_t* uart_router_get_usb_stream();
uint32_t uart_router_get_last_received_tick();
void uart_router_update_last_received_tick(const uint32_t current_tick);
void uart_router_usb_send_flash_ok();
void uart_router_usb_send_ready();
void uart_router_usb_reply_ok();
void uart_router_send_string(UART_HandleTypeDef* huart, const char* str);
void uart_router_start_receive(uart_stream_t* uart_stream);
void uart_router_usb_reinit(const uint32_t HAL_word_length, const uint32_t HAL_parity);
void uart_router_huart_reinit(uart_stream_t* stream);
void uart_router_reset_stream(uart_stream_t* stream);

extern uint32_t last_receive_tick;

#endif
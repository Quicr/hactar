#ifndef UART_STREAM_H
#define UART_STREAM_H

#include "stm32f0xx_hal.h"

#include "state.h"

#define COMMAND_TIMEOUT 1000
#define TRANSMISSION_TIMEOUT 30000

typedef struct _uart_stream_t
{
    UART_HandleTypeDef* from_uart;
    UART_HandleTypeDef* to_uart;
    uint8_t* rx_buffer;
    uint16_t rx_buffer_size;
    uint8_t* tx_buffer;
    uint16_t tx_buffer_size;
    uint16_t rx_read;
    uint16_t tx_write;
    uint16_t tx_read;
    uint8_t tx_read_overflow; // TODO remove
    uint8_t tx_free;
    uint16_t pending_bytes;
    uint8_t bytes_ready; // Data received without hitting half or complete buff
    uint8_t first_flow; // TODO remove
    uint32_t last_transmission_time;
    uint8_t has_received;
    uint8_t is_listening;
    uint8_t command_complete;
} uart_stream_t;

// Commands
static const char ui_upload_cmd [] = "ui_upload";
static const char net_upload_cmd [] = "net_upload";
static const char debug_cmd [] = "debug";
static const char ui_debug_cmd [] = "ui_debug";
static const char net_debug_cmd [] = "net_debug";
static const char reset_cmd [] = "reset";
static const char reset_net [] = "reset_net";

static const uint8_t ACK [] = { 0x79 };
static const uint8_t READY [] = { 0x80 };
static const uint8_t NACK [] = { 0x1F };
static const uint8_t HELLO [] = "WHO ARE YOU?";
static const uint8_t HELLO_RES [] = "HELLO, I AM A HACTAR DEVICE";


void HandleRx(uart_stream_t* uart_stream, uint16_t num_received);
void HandleTx(uart_stream_t* uart_stream, enum State* state);
void HandleCommands(uart_stream_t* rx_uart_stream, UART_HandleTypeDef* tx_uart, enum State* state);
void InitUartStreamParameters(uart_stream_t* uart_stream);
void CancelUart(uart_stream_t* uart_stream);
void StartUartReceive(uart_stream_t* uart_stream);

#endif
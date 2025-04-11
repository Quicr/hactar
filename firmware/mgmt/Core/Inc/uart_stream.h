#ifndef UART_STREAM_H
#define UART_STREAM_H

#include "stm32f0xx_hal.h"

#include "state.h"
#include <string.h>

#define COMMAND_TIMEOUT 1000

typedef enum _
{
    Ignore,
    Passthrough,
    Command
} Stream_Mode;

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
    uint8_t free;
} transmit_t;

typedef struct
{
    receive_t rx;
    transmit_t tx;
    Stream_Mode mode;
} uart_stream_t;

typedef struct
{
    uint8_t* buff;
    const uint16_t sz;
    uint16_t idx;
    uint32_t last_update;
} cmd_ring_buff_t;

// Commands
static const uint8_t ui_upload_cmd [] = "ui_upload";
static const uint8_t net_upload_cmd [] = "net_upload";
static const uint8_t debug_cmd [] = "debug";
static const uint8_t ui_debug_cmd [] = "ui_debug";
static const uint8_t net_debug_cmd [] = "net_debug";
static const uint8_t reset_cmd [] = "reset";
static const uint8_t reset_net [] = "reset_net";

static const uint8_t ACK [] = { 0x79 };
static const uint8_t READY [] = { 0x80 };
static const uint8_t NACK [] = { 0x1F };
static const uint8_t HELLO [] = "WHO ARE YOU?";
static const uint8_t HELLO_RES [] = "HELLO, I AM A HACTAR DEVICE";

void Receive(uart_stream_t* stream, uint16_t num_received);
void HandleCommands(uart_stream_t* stream, cmd_ring_buff_t* cmd_ring, enum State* state);
void HandleTx(uart_stream_t* stream, enum State* state);
void Transmit(uart_stream_t* stream, enum State* state);
void InitUartStreamParameters(uart_stream_t* stream);
void CancelUart(uart_stream_t* stream);
void StartUartReceive(uart_stream_t* stream);

#endif
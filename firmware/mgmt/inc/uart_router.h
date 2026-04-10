#ifndef UART_ROUTER_H
#define UART_ROUTER_H

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"

#define NET_UART_BUFF_SZ 2048
#define USB_UART_BUFF_SZ 2048
#define UI_UART_BUFF_SZ 1024
#define INTERNAL_BUFF_SZ 512
#define PACKET_SZ 512
#define COMMAND_TIMEOUT 1000

#define LINK_SYNC_WORD_LEN 4
#define TYPE_LEN 2
#define LENGTH_LEN 4
#define HEADER_LEN TYPE_LEN + LENGTH_LEN

static const uint8_t Link_Sync_Word[LINK_SYNC_WORD_LEN] = {0x4C, 0x49, 0x4E, 0x4B};

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
    uint16_t write;
    uint16_t read;

    uint32_t total_tlv_read;
    uint16_t num_read;
    uint8_t sync_matched;
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

typedef struct
{
    uint16_t type;
    uint32_t len;
    uint8_t value[PACKET_SZ];
} tlv_packet_t;

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received);
void uart_router_copy_to_tx(transmit_t* tx, const uint8_t* buff, const uint16_t num_bytes);
void uart_router_copy_byte_to_tx(transmit_t* tx, const uint8_t byte);
void uart_router_copy_string_to_tx(transmit_t* tx, const char* str);
void uart_router_tx_isr(uart_stream_t* tx);
void uart_router_transmit(uart_stream_t* tx);
void uart_router_perform_transmit(uart_stream_t* tx);
uart_stream_t* uart_router_get_ui_stream();
uart_stream_t* uart_router_get_net_stream();
uart_stream_t* uart_router_get_usb_stream();
uint32_t uart_router_get_last_received_tick();
void uart_router_update_last_received_tick(const uint32_t current_tick);
void uart_router_usb_send_flash_ok();
void uart_router_usb_send_ready();
void uart_router_usb_reply_ok();
void uart_router_send_byte(UART_HandleTypeDef* huart, const uint8_t byte);
void uart_router_send_string(UART_HandleTypeDef* huart, const char* str);
void uart_router_start_receive(uart_stream_t* uart_stream);
void uart_router_update_baudrate(uart_stream_t* stream, const uint32_t baudrate);
void uart_router_usb_update_reinit(const uint32_t HAL_word_length, const uint32_t HAL_parity);
void uart_router_reinit_stream(uart_stream_t* uart_stream);
void uart_router_huart_reinit(uart_stream_t* stream);
void uart_router_reset_stream(uart_stream_t* stream);
void uart_router_reply_ack();

void uart_router_read_usb();
void uart_router_read_ui();
void uart_router_read_net();
uint8_t uart_router_read_tlv(uart_stream_t* stream, tlv_packet_t* packet);
void uart_router_write_tlv(transmit_t* tx, tlv_packet_t* packet);

extern uint32_t last_receive_tick;

#endif

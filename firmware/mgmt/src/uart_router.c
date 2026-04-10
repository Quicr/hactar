#include "uart_router.h"
#include "command_handler.h"
#include "main.h"
#include "stm32f0xx_hal_uart.h"
#include <stdint.h>
#include <string.h>

// https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define min(a, b)                                                                                  \
    ({                                                                                             \
        __typeof__(a) _a = (a);                                                                    \
        __typeof__(b) _b = (b);                                                                    \
        _a < _b ? _a : _b;                                                                         \
    })

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

#define usb_uart huart1
#define ui_uart huart2
#define net_uart huart3

static uint8_t ui_rx_buff[UI_UART_BUFF_SZ] = {0};
static uint8_t ui_tx_buff[UI_UART_BUFF_SZ] = {0};

static uint8_t net_rx_buff[NET_UART_BUFF_SZ] = {0};
static uint8_t net_tx_buff[NET_UART_BUFF_SZ] = {0};

static uint8_t usb_rx_buff[USB_UART_BUFF_SZ] = {0};
static uint8_t usb_tx_buff[USB_UART_BUFF_SZ] = {0};

static uint8_t internal_buff[INTERNAL_BUFF_SZ] = {0};

uart_stream_t usb_stream = {
    .uart = &usb_uart,
    .rx =
        {
            .buff = usb_rx_buff,
            .size = USB_UART_BUFF_SZ,
            .write = 0,
            .total_tlv_read = 0,
            .num_read = 0,
            .sync_matched = 0,
        },
    .tx =
        {
            .buff = usb_tx_buff,
            .size = USB_UART_BUFF_SZ,
            .read = 0,
            .write = 0,
            .num_sending = 0,
            .free = 1,
        },
    .path = Tx_Path_None,
};

uart_stream_t ui_stream = {
    .uart = &ui_uart,
    .rx =
        {
            .buff = ui_rx_buff,
            .size = UI_UART_BUFF_SZ,
            .write = 0,
            .total_tlv_read = 0,
            .num_read = 0,
            .sync_matched = 0,
        },
    .tx =
        {
            .buff = ui_tx_buff,
            .size = UI_UART_BUFF_SZ,
            .read = 0,
            .write = 0,
            .num_sending = 0,
            .free = 1,
        },
    .path = Tx_Path_None,
};

uart_stream_t net_stream = {
    .uart = &net_uart,
    .rx =
        {
            .buff = net_rx_buff,
            .size = NET_UART_BUFF_SZ,
            .write = 0,
            .total_tlv_read = 0,
            .num_read = 0,
            .sync_matched = 0,
        },
    .tx =
        {
            .buff = net_tx_buff,
            .size = NET_UART_BUFF_SZ,
            .read = 0,
            .write = 0,
            .num_sending = 0,
            .free = 1,
        },
    .path = Tx_Path_None,
};

static transmit_t internal_tx = {
    .buff = internal_buff,
    .size = INTERNAL_BUFF_SZ,
    .read = 0,
    .write = 0,
    .num_sending = 0,
    .free = 1,
};

static uint8_t Ok_Byte[] = {0x80};
static uint8_t Ready_Byte[] = {0x81};
static uint8_t Ok_Ascii[3] = "Ok\n";
static uint8_t Ack = 0x82;
static uint8_t Nack = 0x83;

// static const uint8_t Link_Sync_Word[4] = {0x4C, 0x49, 0x4E, 0x4B};

uint32_t last_receive_tick = 0;

// Assumes unsent > 0
static uint8_t read_from_tx(transmit_t* tx, uint32_t* num_read)
{
    if (tx->read >= tx->size)
    {
        tx->read = 0;
    }

    (*num_read) += 1;

    return tx->buff[tx->read++];
}

static uint8_t tlv_byte_read_rx(receive_t* rx)
{
    if (rx->read >= rx->size)
    {
        rx->read = 0;
    }

    rx->total_tlv_read += 1;

    return rx->buff[rx->read++];
}

static uint8_t byte_read_rx(receive_t* rx)
{
    if (rx->read >= rx->size)
    {
        rx->read = 0;
    }

    return rx->buff[rx->read++];
}

static uint16_t get_tx_distance(transmit_t* tx)
{
    if (tx->write > tx->read)
    {
        // read head is directly behind write head just need the difference
        return tx->write - tx->read;
    }

    // the read head is ahead of the write head, so we read until the end of the buffer
    // and wrap around on the isr
    return tx->size - tx->read;
}

static uint16_t get_rx_distance(receive_t* tx)
{
    if (tx->write > tx->read)
    {
        // read head is directly behind write head just need the difference
        return tx->write - tx->read;
    }

    // the read head is ahead of the write head, so we read until the end of the buffer
    // and wrap around on the isr
    return tx->size - tx->read;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t rx_idx)
{
    // There are 3 conditions that call this function.

    // 1. Half complete -- The rx buffer is half full
    // 2. rx complete -- The rx buffer is full
    // 3. idle  -- Nothing has been received in awhile

    // HAL_GPIO_TogglePin(LEDB_G_GPIO_Port, LEDB_G_Pin);

    __disable_irq();

    if (huart->Instance == net_uart.Instance)
    {
        uart_router_rx_isr(&net_stream, rx_idx);
    }
    else if (huart->Instance == ui_uart.Instance)
    {
        uart_router_rx_isr(&ui_stream, rx_idx);
    }
    else if (huart->Instance == usb_uart.Instance)
    {
        last_receive_tick = HAL_GetTick();
        uart_router_rx_isr(&usb_stream, rx_idx);
    }

    __enable_irq();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // HAL_GPIO_TogglePin(LEDB_B_GPIO_Port, LEDB_B_Pin);

    __disable_irq();
    if (!net_stream.tx.free && huart->Instance == net_stream.uart->Instance)
    {
        uart_router_tx_isr(&net_stream);
    }
    else if (!ui_stream.tx.free && huart->Instance == ui_stream.uart->Instance)
    {
        uart_router_tx_isr(&ui_stream);
    }
    else if (!usb_stream.tx.free && huart->Instance == usb_stream.uart->Instance)
    {
        uart_router_tx_isr(&usb_stream);
    }

    __enable_irq();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    __disable_irq();

    if (huart->Instance == net_stream.uart->Instance)
    {
        // uart_router_reinit_stream(&net_stream);
    }
    else if (huart->Instance == ui_stream.uart->Instance)
    {
        uart_router_reinit_stream(&ui_stream);
    }
    else if (huart->Instance == usb_stream.uart->Instance)
    {
        uart_router_reinit_stream(&usb_stream);
    }

    __enable_irq();
}

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    const uint16_t num_bytes = num_received - stream->rx.write;

    stream->rx.write += num_bytes;
    // rx read head is at the end
    if (stream->rx.write >= stream->rx.size)
    {
        stream->rx.write = 0;
    }
}

void uart_router_copy_to_tx(transmit_t* tx, const uint8_t* buff, const uint16_t num_bytes)
{
    // If num bytes overflows our buffer, only read the first bytes
    const uint16_t bytes = min(tx->size, num_bytes);

    uint16_t read_idx = 0;
    while (read_idx < bytes)
    {
        // Get the amount we can copy
        const uint16_t to_copy = min(tx->size - tx->write, bytes - read_idx);

        memcpy(tx->buff + tx->write, buff + read_idx, to_copy);

        tx->write += to_copy;
        read_idx += to_copy;

        if (tx->write >= tx->size)
        {
            tx->write = 0;
        }
    }
}

void uart_router_copy_byte_to_tx(transmit_t* tx, const uint8_t byte)
{
    uart_router_copy_to_tx(tx, &byte, 1);
}

void uart_router_copy_string_to_tx(transmit_t* tx, const char* str)
{
    // Get the string len
    size_t len = strlen(str);

    uart_router_copy_to_tx(tx, (const uint8_t*)str, len);
}

void uart_router_tx_isr(uart_stream_t* stream)
{
    stream->tx.read += stream->tx.num_sending;

    if (stream->tx.read >= stream->tx.size)
    {
        stream->tx.read = 0;
    }

    uart_router_perform_transmit(stream);
}

void uart_router_transmit(uart_stream_t* stream)
{
    if (!stream->tx.free)
    {
        return;
    }

    uart_router_perform_transmit(stream);
}

void uart_router_perform_transmit(uart_stream_t* stream)
{
    if (stream->tx.read == stream->tx.write)
    {
        stream->tx.free = 1;
        return;
    }

    stream->tx.free = 0;

    stream->tx.num_sending = get_tx_distance(&stream->tx);

    HAL_UART_Transmit_DMA(stream->uart, stream->tx.buff + stream->tx.read, stream->tx.num_sending);
}

void uart_router_parse_internal(const command_map_t command_map[Cmd_Count])
{
}

uart_stream_t* uart_router_get_ui_stream()
{
    return &ui_stream;
}

uart_stream_t* uart_router_get_net_stream()
{
    return &net_stream;
}

uart_stream_t* uart_router_get_usb_stream()
{
    return &usb_stream;
}

uint32_t uart_router_get_last_received_tick()
{
    return last_receive_tick;
}

void uart_router_update_last_received_tick(const uint32_t current_tick)
{
    last_receive_tick = current_tick;
}

void uart_router_usb_send_flash_ok()
{
    HAL_UART_Transmit(&usb_uart, Ok_Byte, 1, HAL_MAX_DELAY);
}

void uart_router_usb_send_ready()
{
    HAL_UART_Transmit(&usb_uart, Ready_Byte, 1, HAL_MAX_DELAY);
}

void uart_router_usb_reply_ok()
{
    HAL_UART_Transmit(&usb_uart, Ok_Ascii, sizeof(Ok_Ascii), HAL_MAX_DELAY);
}

void uart_router_send_byte(UART_HandleTypeDef* huart, const uint8_t byte)
{
    HAL_UART_Transmit(huart, &byte, 1, HAL_MAX_DELAY);
}

void uart_router_send_string(UART_HandleTypeDef* huart, const char* str)
{
    // Get the len
    uint16_t len = strlen(str);

    HAL_UART_Transmit(huart, (const uint8_t*)str, len, HAL_MAX_DELAY);
}

void uart_router_start_receive(uart_stream_t* uart_stream)
{
    uart_router_reset_stream(uart_stream);

    uint8_t attempt = 0;
    while (attempt++ != 10
           && HAL_OK
                  != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->uart, uart_stream->rx.buff,
                                                  uart_stream->rx.size))
    {
        // Make sure the uart is cancelled, sometimes it doesn't want to cancel
        HAL_UART_Abort(uart_stream->uart);
    }

    if (attempt >= 10)
    {
        Error_Handler();
    }
}

void uart_router_update_baudrate(uart_stream_t* stream, const uint32_t baudrate)
{
    HAL_UART_Abort(stream->uart);

    // De-Init uart for UI upload
    if (HAL_UART_DeInit(stream->uart) != HAL_OK)
    {
        Error_Handler();
    }

    stream->uart->Init.BaudRate = baudrate;

    if (HAL_UART_Init(stream->uart) != HAL_OK)
    {
        Error_Handler();
    }
    uart_router_start_receive(stream);
}

void uart_router_usb_update_reinit(const uint32_t HAL_word_length, const uint32_t HAL_parity)
{
    HAL_UART_Abort(&usb_uart);

    // Init uart1 for UI upload
    if (HAL_UART_DeInit(&usb_uart) != HAL_OK)
    {
        Error_Handler();
    }

    usb_uart.Init.WordLength = HAL_word_length;
    usb_uart.Init.Parity = HAL_parity;

    if (HAL_UART_Init(&usb_uart) != HAL_OK)
    {
        Error_Handler();
    }

    uart_router_start_receive(&usb_stream);
}

void uart_router_reinit_stream(uart_stream_t* stream)
{
    HAL_UART_Abort(stream->uart);

    // Init uart1 for UI upload
    if (HAL_UART_DeInit(stream->uart) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UART_Init(stream->uart) != HAL_OK)
    {
        Error_Handler();
    }

    uart_router_start_receive(stream);
}

void uart_router_reset_stream(uart_stream_t* stream)
{
    stream->rx.num_read = 0;
    stream->rx.read = 0;
    stream->rx.total_tlv_read = 0;
    stream->rx.sync_matched = 0;
    stream->rx.write = 0;

    stream->tx.read = 0;
    stream->tx.write = 0;
    stream->tx.unsent = 0;
    stream->tx.num_sending = 0;
    stream->tx.free = 1;

    stream->rx.write = 0;
}

void uart_router_reply_ack()
{
    const uint16_t reply = AckReply;

    uart_router_copy_to_tx(&usb_stream.tx, Link_Sync_Word, LINK_SYNC_WORD_LEN);

    uart_router_copy_byte_to_tx(&usb_stream.tx, reply & 0xFF);
    uart_router_copy_byte_to_tx(&usb_stream.tx, reply >> 8);

    uart_router_copy_byte_to_tx(&usb_stream.tx, 0);
    uart_router_copy_byte_to_tx(&usb_stream.tx, 0);
    uart_router_copy_byte_to_tx(&usb_stream.tx, 0);
    uart_router_copy_byte_to_tx(&usb_stream.tx, 0);
}

static void init_packet(tlv_packet_t* packet)
{
    packet->len = 0;
    packet->type = 0;
}

void uart_router_read_usb()
{
    static tlv_packet_t packet = {0};

    const uint8_t done = uart_router_read_tlv(&usb_stream, &packet);

    if (!done)
    {
        return;
    }

    command_handle_packet(&packet);
}

uint8_t uart_router_read_raw(uart_stream_t* stream, tlv_packet_t* packet)
{
    receive_t* rx = &stream->rx;
    if (rx->read == rx->write)
    {
        return 0;
    }

    while (rx->read != rx->write && rx->num_read < PACKET_SZ)
    {
        packet->value[rx->num_read++] = tlv_byte_read_rx(rx);
    }

    packet->len = rx->num_read;

    rx->num_read = 0;
    rx->total_tlv_read = 0;

    return packet->len > 0;
}

void uart_router_read_ui()
{
    static tlv_packet_t packet = {.type = FromUi, .len = 0};

    if (!uart_router_read_raw(&ui_stream, &packet))
    {
        return;
    }

    uart_router_write_tlv(&usb_stream.tx, &packet);
}

void uart_router_read_net()
{
    static tlv_packet_t packet = {.type = FromNet, .len = 0};

    if (!uart_router_read_raw(&ui_stream, &packet))
    {
        return;
    }

    uart_router_write_tlv(&usb_stream.tx, &packet);
}

uint8_t uart_router_read_tlv(uart_stream_t* stream, tlv_packet_t* packet)
{
    // Note, sync word is not part of the header.
    receive_t* rx = &stream->rx;

    while (rx->read != rx->write)
    {
        if (rx->sync_matched < LINK_SYNC_WORD_LEN)
        {
            const uint8_t byte = byte_read_rx(rx);
            if (byte == Link_Sync_Word[rx->sync_matched])
            {
                ++rx->sync_matched;
            }
            else
            {
                rx->sync_matched = byte == Link_Sync_Word[0];
            }
            continue;
        }

        const uint16_t distance = get_rx_distance(rx);

        // If we don't have enough bytes for the length and the command
        if (distance < HEADER_LEN && rx->total_tlv_read == LINK_SYNC_WORD_LEN)
        {
            return 0;
        }

        if (rx->total_tlv_read < HEADER_LEN)
        {
            packet->type = tlv_byte_read_rx(rx);
            packet->type |= tlv_byte_read_rx(rx);

            packet->len = (uint32_t)(tlv_byte_read_rx(rx));
            packet->len |= (uint32_t)(tlv_byte_read_rx(rx) << 8);
            packet->len |= (uint32_t)(tlv_byte_read_rx(rx) << 16);
            packet->len |= (uint32_t)(tlv_byte_read_rx(rx) << 24);

            if (packet->len == 0)
            {
                return 1;
            }
        }

        packet->value[rx->num_read++] = tlv_byte_read_rx(rx);

        if (rx->num_read >= packet->len)
        {
            rx->total_tlv_read = 0;
            rx->num_read = 0;
            rx->sync_matched = 0;

            return 1;
        }
        else if (rx->num_read >= PACKET_SZ)
        {
            // bad data continue
            rx->total_tlv_read = 0;
            rx->num_read = 0;
            rx->sync_matched = 0;
            continue;
        }
    }

    return 0;
}

void uart_router_write_tlv(transmit_t* tx, tlv_packet_t* packet)
{
    uart_router_copy_to_tx(tx, Link_Sync_Word, LINK_SYNC_WORD_LEN);
    uart_router_copy_to_tx(tx, (const uint8_t*)&packet->type, sizeof(packet->type));
    uart_router_copy_to_tx(tx, (const uint8_t*)&packet->len, sizeof(packet->len));
    uart_router_copy_to_tx(tx, packet->value, packet->len);
}

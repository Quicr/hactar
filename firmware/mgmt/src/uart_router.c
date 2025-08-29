#include "uart_router.h"
#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

static UART_HandleTypeDef* usb_uart = &huart1;
static UART_HandleTypeDef* ui_uart = &huart2;
static UART_HandleTypeDef* net_uart = &huart3;

static uint8_t ui_rx_buff[UART_BUFF_SZ] = {0};
static uint8_t ui_tx_buff[UART_BUFF_SZ] = {0};

static uint8_t net_rx_buff[UART_BUFF_SZ] = {0};
static uint8_t net_tx_buff[UART_BUFF_SZ] = {0};

static uint8_t usb_rx_buff[UART_BUFF_SZ] = {0};
static uint8_t usb_tx_buff[UART_BUFF_SZ] = {0};

static uint8_t internal_buff[INTERNAL_BUFF_SZ];

static transmit_t usb_tx = {
    .uart = &huart1,
    .buff = usb_tx_buff,
    .size = UART_BUFF_SZ,
    .read = 0,
    .write = 0,
    .unsent = 0,
    .num_sending = 0,
    .free = 1,
};

static receive_t usb_rx = {
    .uart = &huart1,
    .buff = usb_rx_buff,
    .size = UART_BUFF_SZ,
    .idx = 0,
};

uart_stream_t usb_stream = {.rx = &usb_rx, .tx = &usb_tx, .path = Tx_Path_None};

static transmit_t ui_tx = {
    .uart = &huart1,
    .buff = ui_tx_buff,
    .size = UART_BUFF_SZ,
    .read = 0,
    .write = 0,
    .unsent = 0,
    .num_sending = 0,
    .free = 1,
};

static receive_t ui_rx = {
    .uart = &huart2,
    .buff = ui_rx_buff,
    .size = UART_BUFF_SZ,
    .idx = 0,
};

uart_stream_t ui_stream = {.rx = &ui_rx, .tx = &ui_tx, .path = Tx_Path_None};

static transmit_t net_tx = {
    .uart = &huart1,
    .buff = net_tx_buff,
    .size = UART_BUFF_SZ,
    .read = 0,
    .write = 0,
    .unsent = 0,
    .num_sending = 0,
    .free = 1,
};

static receive_t net_rx = {
    .uart = &huart3,
    .buff = net_rx_buff,
    .size = UART_BUFF_SZ,
    .idx = 0,
};

uart_stream_t net_stream = {.rx = &net_rx, .tx = &net_tx, .path = Tx_Path_None};

static transmit_t internal_tx = {
    .uart = NULL,
    .buff = internal_buff,
    .size = INTERNAL_BUFF_SZ,
    .read = 0,
    .write = 0,
    .unsent = 0,
    .num_sending = 0,
    .free = 1,
};

static uint32_t last_receive_tick = 0;

static uint8_t Ok_Byte[] = {0x80};
static uint8_t Ready_Byte[] = {0x81};

// Assumes unsent > 0
static uint8_t read_from_tx(transmit_t* tx, int32_t* num_read)
{
    if (tx->read >= tx->size)
    {
        tx->read = 0;
    }

    --tx->unsent;

    (*num_read) += 1;

    return tx->buff[tx->read++];
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t rx_idx)
{
    // There are 3 conditions that call this function.

    // 1. Half complete -- The rx buffer is half full
    // 2. rx complete -- The rx buffer is full
    // 3. idle  -- Nothing has been received in awhile

    HAL_GPIO_TogglePin(LEDB_G_GPIO_Port, LEDB_G_Pin);
    last_receive_tick = HAL_GetTick();

    __disable_irq();

    if (huart->Instance == net_uart->Instance)
    {
        uart_router_rx_isr(&net_stream, rx_idx);
    }
    else if (huart->Instance == ui_uart->Instance)
    {
        uart_router_rx_isr(&ui_stream, rx_idx);
    }
    else if (huart->Instance == usb_uart->Instance)
    {
        uart_router_rx_isr(&usb_stream, rx_idx);
    }

    __enable_irq();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    HAL_GPIO_TogglePin(LEDB_B_GPIO_Port, LEDB_B_Pin);

    __disable_irq();
    if (!net_tx.free && huart->Instance == net_tx.uart->Instance)
    {
        uart_router_tx_isr(&net_tx);
    }
    else if (!ui_tx.free && huart->Instance == ui_tx.uart->Instance)
    {
        uart_router_tx_isr(&ui_tx);
    }
    else if (!usb_tx.free && huart->Instance == usb_tx.uart->Instance)
    {
        uart_router_tx_isr(&usb_tx);
    }

    __enable_irq();
}

void uart_router_rx_isr(uart_stream_t* stream, const uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    const uint16_t num_bytes = num_received - stream->rx->idx;

    // Faster than putting a check inside of the copy loop since this is only
    // checked once per rx event.
    switch (stream->path)
    {
    case Tx_Path_None:
        break;
    case Tx_Path_Usb:
        uart_router_copy_to_tx(&usb_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        break;
    case Tx_Path_Ui:
        uart_router_copy_to_tx(&ui_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        break;
    case Tx_Path_Net:
        uart_router_copy_to_tx(&net_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        break;
    case Tx_Path_Ui_Net:
        uart_router_copy_to_tx(&ui_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        uart_router_copy_to_tx(&net_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        break;
    case Tx_Path_Internal:
        uart_router_copy_to_tx(&internal_tx, stream->rx->buff + stream->rx->idx, num_bytes);
        break;
    }

    stream->rx->idx += num_bytes;
    // rx read head is at the end
    if (stream->rx->idx >= stream->rx->size)
    {
        stream->rx->idx = 0;
    }
}

void uart_router_copy_to_tx(transmit_t* tx, const uint8_t* buff, const uint16_t num_bytes)
{
    if (tx->write + num_bytes >= tx->size)
    {
        const uint16_t wrap_bytes = tx->size - tx->write;

        memcpy(tx->buff + tx->write, buff, wrap_bytes);
        memcpy(tx->buff, buff + wrap_bytes, num_bytes - wrap_bytes);

        tx->write = 0;
    }
    else
    {
        memcpy(tx->buff + tx->write, buff, num_bytes);
    }

    // Copy bytes to tx buffer
    tx->unsent += num_bytes;
    tx->write += num_bytes;
}

void uart_router_copy_string_to_tx(transmit_t* tx, const char* str)
{
    // Get the string len
    size_t len = strlen(str);

    uart_router_copy_to_tx(tx, (const uint8_t*)str, len);
}

void uart_router_tx_isr(transmit_t* tx)
{
    tx->unsent -= tx->num_sending;
    tx->read += tx->num_sending;

    if (tx->read >= tx->size)
    {
        tx->read = 0;
    }

    uart_router_transmit(tx);
}

void uart_router_transmit(transmit_t* tx)
{
    if (tx->unsent == 0)
    {
        tx->free = 1;
        return;
    }

    tx->free = 0;

    tx->num_sending = tx->unsent;
    if (tx->read + tx->num_sending >= tx->size)
    {
        tx->num_sending = tx->size - tx->read;
    }

    HAL_UART_Transmit_DMA(tx->uart, tx->buff + tx->read, tx->num_sending);
}

void uart_router_parse_internal(const command_map_t command_map[Cmd_Count])
{
    // Parse our internal, if it is a zero length TLV then we can just assume it is a command
    // otherwise... I dunno right now.

    const uint16_t Header_Bytes = 3;

    static Command command = 0;
    static uint16_t len = 0;
    static int32_t num_read = 0;

    static uint16_t packet_idx = 0;
    static uint8_t packet[PACKET_SZ] = {0};

    while (internal_tx.unsent > 0)
    {
        // If we don't have enough bytes for the length and the command
        if (internal_tx.unsent < 3 && num_read != 0)
        {
            break;
        }

        if (num_read == 0)
        {
            command = read_from_tx(&internal_tx, &num_read);
            len = read_from_tx(&internal_tx, &num_read);
            len |= read_from_tx(&internal_tx, &num_read) << 8;
        }

        if (command >= Cmd_Count)
        {
            // bad data continue
            num_read = 0;
            continue;
        }

        // Get the type
        if (len == 0)
        {
            switch (command)
            {
            case Cmd_To_Ui:
            case Cmd_To_Net:
            case Cmd_Loopback:
            {
                // Bad data continue;
                num_read = 0;
                break;
            }
            default:
            {
                // Is command, check if the idx matches the type.
                if (command_map[command].command == command
                    && command_map[command].callback != NULL)
                {
                    command_map[command].callback(command_map[command].usr_arg);
                }
                break;
            }
            }
            num_read = 0;
        }
        else if (num_read - Header_Bytes >= len)
        {
            // Send the packet to ui or net
            switch (command)
            {
            case Cmd_To_Ui:
            {
                // TODO

                break;
            }
            case Cmd_To_Net:
            {
                break;
            }
            case Cmd_Loopback:
            {
                break;
            }
            default:
                break;
            }

            num_read = 0;
            packet_idx = 0;
        }
        else
        {
            if (packet_idx >= PACKET_SZ)
            {
                packet_idx = 0;
                num_read = 0;
                continue;
            }

            // Read more bytes!
            packet[packet_idx++] = read_from_tx(&internal_tx, &num_read);
        }
    }
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

void uart_router_send_flash_ok()
{
    uart_router_copy_to_tx(usb_stream.tx, Ok_Byte, 1);
}

void uart_router_send_ready()
{
    uart_router_copy_to_tx(usb_stream.tx, Ready_Byte, 1);
}

void uart_router_usb_reinit(const uint32_t HAL_word_length, const uint32_t HAL_parity)
{
    UART_HandleTypeDef* uart = usb_stream.rx->uart;

    HAL_UART_Abort(uart);

    // Init uart1 for UI upload
    if (HAL_UART_DeInit(uart) != HAL_OK)
    {
        Error_Handler();
    }

    uart->Init.WordLength = HAL_word_length;
    uart->Init.Parity = HAL_parity;

    if (HAL_UART_Init(uart) != HAL_OK)
    {
        Error_Handler();
    }

    uart_router_reset_stream(&usb_stream);

    HAL_UARTEx_ReceiveToIdle_DMA(uart, usb_rx.buff, usb_rx.size);
}

void uart_router_reset_stream(uart_stream_t* stream)
{
    stream->tx->free = 1;
    stream->tx->num_sending = 0;
    stream->tx->read = 0;
    stream->tx->unsent = 0;
    stream->tx->write = 0;

    stream->rx->idx = 0;
}
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

uart_stream_t usb_stream = {.rx = &usb_rx, .tx = &usb_tx, .mode = Ignore};

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

uart_stream_t ui_stream = {.rx = &ui_rx, .tx = &ui_tx, .mode = Ignore};

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

uart_stream_t net_stream = {.rx = &net_rx, .tx = &net_tx, .mode = Ignore};

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
    switch (stream->direction)
    {
    case None:
        break;
    case Usb:
        uart_router_copy_rx_data(stream->rx, &usb_tx, num_bytes);
        break;
    case Ui:
        uart_router_copy_rx_data(stream->rx, &ui_tx, num_bytes);
        break;
    case Net:
        uart_router_copy_rx_data(stream->rx, &net_tx, num_bytes);
        break;
    case Ui_Net:
        uart_router_copy_rx_data(stream->rx, &ui_tx, num_bytes);
        uart_router_copy_rx_data(stream->rx, &net_tx, num_bytes);
        break;
    case Internal:
        uart_router_copy_rx_data(stream->rx, &internal_tx, num_bytes);
        break;
    }

    stream->rx->idx += num_bytes;
    // rx read head is at the end
    if (stream->rx->idx >= stream->rx->size)
    {
        stream->rx->idx = 0;
    }
}

void uart_router_copy_rx_data(receive_t* rx, transmit_t* tx, const uint16_t num_bytes)
{
    if (tx->write + num_bytes >= tx->size)
    {
        const uint16_t wrap_bytes = tx->size - tx->write;

        memcpy(tx->buff + tx->write, rx->buff + rx->idx, wrap_bytes);
        memcpy(tx->buff, rx->buff + rx->idx + wrap_bytes, num_bytes - wrap_bytes);

        tx->write = 0;
    }
    else
    {
        memcpy(tx->buff + tx->write, rx->buff + rx->idx, num_bytes);
    }

    // Copy bytes to tx buffer
    tx->unsent += num_bytes;
    tx->write += num_bytes;
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
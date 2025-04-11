#include "uart_stream.h"

#include "chip_control.h"
#include "main.h"

void Receive(uart_stream_t* stream, uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    const uint16_t pending_bytes = num_received - stream->rx.idx;
    uint16_t num_bytes = pending_bytes;

    // Faster than putting a check inside of the copy loop since this is only
    // checked once per rx event.
    if (stream->tx.write + num_bytes > stream->tx.size)
    {
        const uint16_t bytes = stream->tx.size - stream->tx.write;

        memcpy(stream->tx.buff + stream->tx.write,
            stream->rx.buff + stream->rx.idx,
            bytes);

        stream->rx.idx += bytes;
        stream->tx.write = 0;
        num_bytes -= bytes;
    }

    // Copy bytes to tx buffer
    memcpy(stream->tx.buff + stream->tx.write,
        stream->rx.buff + stream->rx.idx,
        num_bytes);

    stream->rx.idx += num_bytes;
    stream->tx.write += num_bytes;

    stream->tx.unsent += num_bytes;

    // rx read head is at the end
    if (stream->rx.idx >= stream->rx.size)
    {
        stream->rx.idx = 0;
    }
}

void HandleTx(uart_stream_t* stream, enum State* state)
{
    switch (stream->mode)
    {
    case (Passthrough):
    {
        if (!stream->tx.free)
        {
            return;
        }
        Transmit(stream, state);
        break;
    }
    case (Command):
    {

        break;
    }
    case (Ignore):
    {
        break;
    }
    default:
    {
        Error_Handler();
        break;
    }
    }
}

// Both a callback and a normal function
void Transmit(uart_stream_t* stream, enum State* state)
{
    if (stream->tx.unsent == 0)
    {
        stream->tx.free = 1;
        return;
    }
    stream->tx.free = 0;

    uint16_t bytes = stream->tx.unsent;
    if (stream->tx.read + bytes >= stream->tx.size)
    {
        bytes = stream->tx.size - stream->tx.read;
    }

    // Transmit
    HAL_UART_Transmit_DMA(stream->tx.uart,
        stream->tx.buff + stream->tx.read, bytes);

    stream->tx.unsent -= bytes;
    stream->tx.read += bytes;
    if (stream->tx.read >= stream->tx.size)
    {
        stream->tx.read = 0;
    }
}

void HandleCommands(uart_stream_t* stream,
    cmd_ring_buff_t* cmd_ring,
    enum State* state)
{
    static uint8_t ready = 0;
    // I don't care about performance as much in this function as
    // commands should be pretty short.
    while (stream->tx.unsent > 0)
    {
        if (stream->tx.read >= stream->tx.size)
        {
            stream->tx.read = 0;
        }

        if (cmd_ring->idx >= cmd_ring->sz)
        {
            ready = 1;
            break;
        }

        const uint8_t ch = stream->tx.buff[stream->tx.read++];
        --stream->tx.unsent;
        if (ch)
        {
            cmd_ring->buff[cmd_ring->idx++] = ch;
            cmd_ring->last_update = HAL_GetTick();
        }
        else
        {
            if (cmd_ring->idx > 0)
            {
                ready = 1;
            }
            break;
        }
    }

    if (ready || HAL_GetTick() - cmd_ring->last_update >= COMMAND_TIMEOUT)
    {
        if (strcmp((const char*)cmd_ring->buff, (const char*)ui_upload_cmd) == 0)
        {
            // Echo back
            HAL_UART_Transmit(stream->rx.uart, ACK, 1, HAL_MAX_DELAY);
            *state = UI_Upload_Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)net_upload_cmd) == 0)
        {
            HAL_UART_Transmit(stream->rx.uart, ACK, 1, HAL_MAX_DELAY);
            *state = Net_Upload_Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)debug_cmd) == 0)
        {
            *state = Debug_Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)ui_debug_cmd) == 0)
        {
            *state = UI_Debug_Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)net_debug_cmd) == 0)
        {
            *state = Net_Debug_Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)reset_cmd) == 0)
        {
            *state = Reset;
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)reset_net) == 0)
        {
            NetNormalMode();
        }
        else if (strcmp((const char*)cmd_ring->buff, (const char*)HELLO) == 0)
        {
            HAL_UART_Transmit(stream->rx.uart, HELLO_RES, 28, HAL_MAX_DELAY);
        }

        // Clear the ring
        for (uint16_t i = 0; i < cmd_ring->idx; ++i)
        {
            cmd_ring->buff[i] = 0;
        }
        cmd_ring->last_update = HAL_GetTick();
        cmd_ring->idx = 0;
        ready = 0;
    }
}

void InitUartStreamParameters(uart_stream_t* uart_stream)
{
    uart_stream->rx.idx = 0;

    uart_stream->tx.read = 0;
    uart_stream->tx.write = 0;
    uart_stream->tx.unsent = 0;
    uart_stream->tx.free = 1;

    uart_stream->mode = Ignore;
}

void CancelUart(uart_stream_t* uart_stream)
{
    HAL_UART_Abort(uart_stream->rx.uart);
}

void StartUartReceive(uart_stream_t* uart_stream)
{
    uint8_t attempt = 0;
    while (attempt++ != 10 &&
        HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->rx.uart, uart_stream->rx.buff, uart_stream->rx.size))
    {
        // Make sure the uart is cancelled, sometimes it doesn't want to cancel
        CancelUart(uart_stream);
    }

    if (attempt >= 10)
    {
        Error_Handler();
    }
}

// TODO function that will read from tx and do all of the processing from there?
// TODO make the function inline
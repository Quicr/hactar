#include "uart_stream.h"
#include "chip_control.h"
#include "main.h"

#define COMMAND_BUFF_SZ 32

static uint8_t quiet = 0;

void Receive(uart_stream_t* stream, uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    uint16_t num_bytes = num_received - stream->rx.idx;

    // Faster than putting a check inside of the copy loop since this is only
    // checked once per rx event.
    if (stream->tx.write + num_bytes >= stream->tx.size)
    {
        const uint16_t bytes = stream->tx.size - stream->tx.write;

        if (stream->mode != Ignore)
        {
            memcpy(stream->tx.buff + stream->tx.write, stream->rx.buff + stream->rx.idx, bytes);
            stream->tx.unsent += bytes;
        }

        stream->rx.idx += bytes;
        stream->tx.write = 0;
        num_bytes -= bytes;
    }

    if (stream->mode != Ignore)
    {
        // Copy bytes to tx buffer
        memcpy(stream->tx.buff + stream->tx.write, stream->rx.buff + stream->rx.idx, num_bytes);
        stream->tx.unsent += num_bytes;
    }

    stream->rx.idx += num_bytes;
    stream->tx.write += num_bytes;

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
        if (stream->tx.uart->gState != HAL_UART_STATE_READY && !stream->tx.free)
        {
            return;
        }
        Transmit(stream, state);
        break;
    }
    case (Command):
    {
        HandleCommands(stream, state);
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

void TxISR(uart_stream_t* stream, enum State* state)
{
    stream->tx.unsent -= stream->tx.num_sending;
    stream->tx.read += stream->tx.num_sending;
    if (stream->tx.read >= stream->tx.size)
    {
        stream->tx.read = 0;
    }

    Transmit(stream, state);
}

// Both a callback and a normal function
void Transmit(uart_stream_t* stream, enum State* state)
{
    if (stream->tx.unsent == 0 || quiet)
    {
        stream->tx.free = 1;
        return;
    }
    stream->tx.free = 0;

    stream->tx.num_sending = stream->tx.unsent;
    if (stream->tx.read + stream->tx.num_sending >= stream->tx.size)
    {
        stream->tx.num_sending = stream->tx.size - stream->tx.read;
    }

    // Transmit
    HAL_UART_Transmit_DMA(stream->tx.uart, stream->tx.buff + stream->tx.read,
                          stream->tx.num_sending);
}

void HandleCommands(uart_stream_t* stream, enum State* state)
{
    static uint8_t ready = 0;
    static uint8_t cmd_buff[COMMAND_BUFF_SZ] = {0};
    static uint16_t idx = 0;
    static uint32_t last_update = 0;

    // I don't care about performance as much in this function as
    // commands should be pretty short.
    while (stream->tx.unsent > 0)
    {
        if (stream->tx.read >= stream->tx.size)
        {
            stream->tx.read = 0;
        }

        if (idx >= COMMAND_BUFF_SZ)
        {
            ready = 1;
            break;
        }

        const uint8_t ch = stream->tx.buff[stream->tx.read++];
        --stream->tx.unsent;
        if (ch)
        {
            cmd_buff[idx++] = ch;
            last_update = HAL_GetTick();
        }
        else
        {
            if (idx > 0)
            {
                ready = 1;
            }
            break;
        }
    }

    if (ready || HAL_GetTick() - last_update >= COMMAND_TIMEOUT)
    {
        if (strcmp((const char*)cmd_buff, (const char*)ui_upload_cmd) == 0)
        {
            *state = UI_Upload;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)net_upload_cmd) == 0)
        {
            *state = Net_Upload;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)debug_cmd) == 0)
        {
            *state = Debug;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)ui_debug_cmd) == 0)
        {
            *state = UI_Debug;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)net_debug_cmd) == 0)
        {
            *state = Net_Debug;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)reset_cmd) == 0)
        {
            *state = Normal;
        }
        else if (strcmp((const char*)cmd_buff, (const char*)reset_net) == 0)
        {
            NetNormalMode();
        }
        else if (strcmp((const char*)cmd_buff, (const char*)HELLO) == 0)
        {
            quiet = 1;
            while (!stream->tx.free)
            {
                __NOP();
            }
            HAL_UART_Transmit(stream->rx.uart, HELLO_RES, 28, HAL_MAX_DELAY);
            quiet = 0;
        }

        // Clear the ring
        for (uint16_t i = 0; i < idx; ++i)
        {
            cmd_buff[i] = 0;
        }
        last_update = HAL_GetTick();
        idx = 0;
        ready = 0;
    }
}

void InitUartStream(uart_stream_t* stream)
{
    stream->rx.idx = 0;

    stream->tx.read = 0;
    stream->tx.write = 0;
    stream->tx.unsent = 0;
    stream->tx.free = 1;
}

void StartUartReceive(uart_stream_t* uart_stream)
{
    uint8_t attempt = 0;
    while (attempt++ != 10
           && HAL_OK
                  != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->rx.uart, uart_stream->rx.buff,
                                                  uart_stream->rx.size))
    {
        // Make sure the uart is cancelled, sometimes it doesn't want to cancel
        HAL_UART_Abort(uart_stream->rx.uart);
    }

    if (attempt >= 10)
    {
        Error_Handler();
    }
}

void RestartUartStream(uart_stream_t* stream)
{
    InitUartStream(stream);
    StartUartReceive(stream);
}

// TODO function that will read from tx and do all of the processing from there?
// TODO make the function inline
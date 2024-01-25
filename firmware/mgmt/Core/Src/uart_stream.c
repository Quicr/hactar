#include "uart_stream.h"

void HandleRx(uart_stream_t* rx_stream, uint16_t num_received)
{
    // Calculate the number of bytes have occurred since the last event
    const uint16_t pending_bytes = num_received - rx_stream->rx_read;
    uint16_t num_bytes = pending_bytes;

    // Faster than putting a check inside of the copy loop since this is only
    // checked once per rx event.
    if (rx_stream->tx_write + pending_bytes > rx_stream->tx_buffer_size)
    {
        uint16_t tx_remaining_space =
            rx_stream->tx_buffer_size - rx_stream->tx_write;

        size_t bytes = tx_remaining_space < pending_bytes ? tx_remaining_space : pending_bytes;
        memcpy(rx_stream->tx_buffer + rx_stream->tx_write,
            rx_stream->rx_buffer + rx_stream->rx_read,
            bytes);

        rx_stream->rx_read += bytes;
        rx_stream->tx_write = 0;
        num_bytes -= bytes;
    }

    // Copy bytes to tx buffer
    memcpy(rx_stream->tx_buffer + rx_stream->tx_write,
        rx_stream->rx_buffer + rx_stream->rx_read,
        num_bytes);

    rx_stream->rx_read += num_bytes;
    rx_stream->tx_write += num_bytes;

    // rx read head is at the end
    if (rx_stream->rx_read == rx_stream->rx_buffer_size)
    {
        rx_stream->rx_read = 0;
    }

    if (rx_stream->from_uart->RxEventType == HAL_UART_RXEVENT_IDLE)
    {
        // Set the idle receive flag
        rx_stream->idle_receive = 1;
    }

    // Update the number of pending bytes
    rx_stream->pending_bytes += pending_bytes;
    rx_stream->has_received = 1;
    rx_stream->last_transmission_time = HAL_GetTick();
}

void HandleTx(uart_stream_t* tx_stream, enum State* state)
{
    if (tx_stream->pending_bytes > 0 && tx_stream->tx_free)
    {
        if (tx_stream->pending_bytes >= tx_stream->rx_buffer_size || tx_stream->idle_receive || tx_stream->tx_read_overflow)
        {
            uint16_t send_bytes = tx_stream->rx_buffer_size;

            // Should only occur on an idle
            if (tx_stream->idle_receive || tx_stream->tx_read_overflow)
            {
                send_bytes = tx_stream->pending_bytes;
                tx_stream->tx_read_overflow = 0;
            }

            if (send_bytes > tx_stream->tx_buffer_size - tx_stream->tx_read)
            {
                send_bytes = tx_stream->tx_buffer_size - tx_stream->tx_read;
                tx_stream->tx_read_overflow = 1;
            }
            else if (send_bytes > tx_stream->pending_bytes)
            {
                send_bytes = tx_stream->pending_bytes;
            }

            // Technically this should be lower, but it makes it work for the ui stream...
            // because... good question
            if (tx_stream->idle_receive && tx_stream->pending_bytes == 0)
            {
                tx_stream->idle_receive = 0;
            }

            tx_stream->tx_free = 0;
            HAL_UART_Transmit_DMA(tx_stream->to_uart, (tx_stream->tx_buffer + tx_stream->tx_read), send_bytes);
            tx_stream->pending_bytes -= send_bytes;
            tx_stream->tx_read += send_bytes;
        }

        if (tx_stream->tx_read >= tx_stream->tx_buffer_size)
        {
            tx_stream->tx_read = 0;
        }
    }

    if (HAL_GetTick() > tx_stream->last_transmission_time + TRANSMISSION_TIMEOUT &&
        state != Debug_Running &&
        state != Reset &&
        state != Running)
    {
        // Clean up and return to reset mode
        state = Reset;
    }
}

void HandlePacketTx(uart_stream_t* tx_stream, enum State* state)
{
    if (tx_stream->pending_bytes > 0 && tx_stream->tx_free)
    {
        if (tx_stream->pending_bytes >= tx_stream->rx_buffer_size || tx_stream->idle_receive || tx_stream->tx_read_overflow)
        {
            uint16_t send_bytes = tx_stream->rx_buffer_size;

            // Should only occur on an idle
            if (tx_stream->idle_receive || tx_stream->tx_read_overflow)
            {
                send_bytes = tx_stream->pending_bytes;
                tx_stream->tx_read_overflow = 0;
            }

            if (send_bytes > tx_stream->tx_buffer_size - tx_stream->tx_read)
            {
                send_bytes = tx_stream->tx_buffer_size - tx_stream->tx_read;
                tx_stream->tx_read_overflow = 1;
            }
            else if (send_bytes > tx_stream->pending_bytes)
            {
                send_bytes = tx_stream->pending_bytes;
            }

            // Technically this should be lower, but it makes it work for the ui stream...
            // because... good question
            if (tx_stream->idle_receive && tx_stream->pending_bytes == 0)
            {
                tx_stream->idle_receive = 0;
            }

            tx_stream->tx_free = 0;
            HAL_UART_Transmit_DMA(tx_stream->to_uart, (tx_stream->tx_buffer + tx_stream->tx_read), send_bytes);
            tx_stream->pending_bytes -= send_bytes;
            tx_stream->tx_read += send_bytes;
        }

        if (tx_stream->tx_read >= tx_stream->tx_buffer_size)
        {
            tx_stream->tx_read = 0;
        }
    }

    if (HAL_GetTick() > tx_stream->last_transmission_time + TRANSMISSION_TIMEOUT &&
        state != Debug_Running &&
        state != Reset &&
        state != Running)
    {
        // Clean up and return to reset mode
        state = Reset;
    }
}

void HandleCommands(uart_stream_t* rx_uart_stream,
    UART_HandleTypeDef* tx_uart,
    enum State* state)
{
    if (rx_uart_stream->pending_bytes > 0)
    {
        if (rx_uart_stream->pending_bytes >= rx_uart_stream->rx_buffer_size || rx_uart_stream->idle_receive || rx_uart_stream->tx_read_overflow)
        {
            rx_uart_stream->has_received = 1;
            uint16_t send_bytes = rx_uart_stream->rx_buffer_size;

            // Should only occur on an idle
            if (rx_uart_stream->idle_receive || rx_uart_stream->tx_read_overflow)
            {
                send_bytes = rx_uart_stream->pending_bytes;
                rx_uart_stream->tx_read_overflow = 0;
            }

            if (send_bytes > rx_uart_stream->tx_buffer_size - rx_uart_stream->tx_read)
            {
                send_bytes = rx_uart_stream->tx_buffer_size - rx_uart_stream->tx_read;
                rx_uart_stream->tx_read_overflow = 1;
            }
            else if (send_bytes > rx_uart_stream->pending_bytes)
            {
                send_bytes = rx_uart_stream->pending_bytes;
            }

            rx_uart_stream->pending_bytes -= send_bytes;
            rx_uart_stream->tx_read += send_bytes;

            if (rx_uart_stream->idle_receive || rx_uart_stream->pending_bytes == 0)
            {
                // End of transmission
                rx_uart_stream->idle_receive = 0;
                rx_uart_stream->command_complete = 1;
            }
        }

        if (rx_uart_stream->tx_read >= rx_uart_stream->tx_buffer_size)
        {
            rx_uart_stream->tx_read = 0;
        }
    }

    if (rx_uart_stream->command_complete ||
        (HAL_GetTick() > rx_uart_stream->last_transmission_time + COMMAND_TIMEOUT
            && rx_uart_stream->has_received))
    {
        // Add a null terminator to the end of the string for strcmp
        rx_uart_stream->tx_buffer[rx_uart_stream->tx_read] = '\0';

        if (strcmp((const char*)rx_uart_stream->tx_buffer, ui_upload_cmd) == 0)
        {
            // Echo back
            HAL_UART_Transmit(tx_uart, ACK, 1, HAL_MAX_DELAY);
            *state = UI_Upload_Reset;
        }
        else if (strcmp((const char*)rx_uart_stream->tx_buffer, net_upload_cmd) == 0)
        {
            HAL_UART_Transmit(tx_uart, ACK, 1, HAL_MAX_DELAY);
            *state = Net_Upload_Reset;
        }
        else if (strcmp((const char*)rx_uart_stream->tx_buffer, debug_cmd) == 0)
        {
            *state = Debug_Reset;
        }
        else if (strcmp((const char*)rx_uart_stream->tx_buffer, reset_cmd) == 0)
        {
            *state = Reset;
        }
        else if (strcmp((const char*)rx_uart_stream->tx_buffer, HELLO) == 0)
        {
            HAL_UART_Transmit(tx_uart, HELLO_RES, 28, HAL_MAX_DELAY);
        }

        // Invalidate the command
        rx_uart_stream->tx_buffer[0] = 0;

        // Return to writing at the start of the buffer
        rx_uart_stream->tx_write = 0;

        // Return to reading the start of the buffer
        rx_uart_stream->tx_read = 0;

        // Reset the command and receive
        rx_uart_stream->command_complete = 0;
        rx_uart_stream->has_received = 0;
    }
}

void InitUartStreamParameters(uart_stream_t* uart_stream)
{
    uart_stream->rx_read = 0;
    uart_stream->tx_write = 0;
    uart_stream->tx_read = 0;
    uart_stream->tx_read_overflow = 0;
    uart_stream->tx_free = 1;
    uart_stream->pending_bytes = 0;
    uart_stream->idle_receive = 0;
    uart_stream->last_transmission_time = HAL_GetTick();
    uart_stream->has_received = 0;
    uart_stream->command_complete = 0;
}

void CancelUart(uart_stream_t* uart_stream)
{
    HAL_UART_Abort_IT(uart_stream->from_uart);
    uart_stream->is_listening = 0;
}

void StartUartReceive(uart_stream_t* uart_stream)
{
    if (uart_stream->is_listening)
    {
        // If this occurs we have a bug
        Error_Handler();
    }

    uint8_t attempt = 0;
    while (attempt++ != 10 &&
        HAL_OK != HAL_UARTEx_ReceiveToIdle_DMA(uart_stream->from_uart, uart_stream->rx_buffer, uart_stream->rx_buffer_size))
    {
        // Make sure the uart is cancelled, sometimes it doesn't want to cancel
        CancelUart(uart_stream);
    }

    if (attempt >= 10)
    {
        Error_Handler();
    }

    uart_stream->is_listening = 1;
}

// TODO function that will read from tx and do all of the processing from there?
// TODO make the function inline
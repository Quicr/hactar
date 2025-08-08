#include "uart_stream.h"
#include "app_mgmt.h"
#include "chip_control.h"
#include "main.h"
#include "ui_mgmt_link.h"

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
    case Configuration:
    {
        if (stream->tx.uart->gState != HAL_UART_STATE_READY && !stream->tx.free)
        {
            return;
        }
        HandleConfiguration(stream, state);
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

uint8_t TryCommand(const char* buff, enum State* state, uart_stream_t* stream)
{
    uint8_t ret = 1;
    if (strcmp(buff, (const char*)ui_upload_cmd) == 0)
    {
        *state = UI_Upload;
    }
    else if (strcmp(buff, (const char*)net_upload_cmd) == 0)
    {
        *state = Net_Upload;
    }
    else if (strcmp(buff, (const char*)debug_cmd) == 0)
    {
        *state = Debug;
    }
    else if (strcmp(buff, (const char*)ui_debug_cmd) == 0)
    {
        *state = UI_Debug;
    }
    else if (strcmp(buff, (const char*)net_debug_cmd) == 0)
    {
        *state = Net_Debug;
    }
    else if (strcmp(buff, (const char*)reset_cmd) == 0)
    {
        *state = Normal;
    }
    else if (strcmp(buff, (const char*)reset_ui) == 0)
    {
        UINormalMode();
    }
    else if (strcmp(buff, (const char*)reset_net) == 0)
    {
        NetNormalMode();
    }
    else if (strcmp(buff, (const char*)configure) == 0)
    {
        HAL_UART_Transmit(stream->rx.uart, (const uint8_t*)"Configuration Mode\n", 19,
                          HAL_MAX_DELAY);
        *state = Configator;
    }
    else if (strcmp(buff, (const char*)HELLO) == 0)
    {
        quiet = 1;
        while (!stream->tx.free)
        {
            __NOP();
        }
        HAL_UART_Transmit(stream->rx.uart, HELLO_RES, 28, HAL_MAX_DELAY);
        quiet = 0;
    }
    else
    {
        ret = 0;
    }

    return ret;
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
        TryCommand((const char*)cmd_buff, state, stream);

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

void HandleConfiguration(uart_stream_t* stream, enum State* state)
{
    static uint8_t ready = 0;
    static uint8_t config_buff[CONFIGURATOR_BUFF_SZ] = {0};
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

        if (idx >= CONFIGURATOR_BUFF_SZ)
        {
            ready = 1;
            break;
        }

        const uint8_t ch = stream->tx.buff[stream->tx.read++];
        --stream->tx.unsent;
        if (ch)
        {
            config_buff[idx++] = ch;
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

    // TODO rewrite this is a poc
    if (idx > 0 && (ready || HAL_GetTick() - last_update >= COMMAND_TIMEOUT))
    {
        uint8_t type = Config_Type_Not_Found;
        size_t config_type_len = 0;
        uint32_t len = 0;

        // Find the next word
        char* next_word = strstr((const char*)(config_buff + config_type_len), " ");

        if (next_word != NULL)
        {
            // Null terminate the space so the strcmp can work.
            *next_word = '\0';
        }

        // Try previous commands first
        if (TryCommand(config_buff, state, stream))
        {
            goto cleanup;
        }

        if (strcmp((const char*)config_buff, (const char*)quit_config) == 0)
        {
            HAL_UART_Transmit(stream->rx.uart, (const uint8_t*)"Leaving configuration mode\n", 27,
                              HAL_MAX_DELAY);
            *state = default_state;
            goto cleanup;
        }
        else if (strcmp((const char*)config_buff, (const char*)set_ssid_0) == 0)
        {
            type = Set_Ssid_0;
            config_type_len = strlen(set_ssid_0);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_pwd_0) == 0)
        {
            type = Set_Pwd_0;
            config_type_len = strlen(set_pwd_0);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_ssid_1) == 0)
        {
            type = Set_Ssid_1;
            config_type_len = strlen(set_ssid_1);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_pwd_1) == 0)
        {
            type = Set_Pwd_1;
            config_type_len = strlen(set_pwd_1);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_ssid_2) == 0)
        {
            type = Set_Ssid_2;
            config_type_len = strlen(set_ssid_2);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_pwd_2) == 0)
        {
            type = Set_Pwd_2;
            config_type_len = strlen(set_pwd_2);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_moq_url) == 0)
        {
            type = Set_Moq_Url;
            config_type_len = strlen(set_moq_url);
        }
        else if (strcmp((const char*)config_buff, (const char*)set_sframe_key) == 0)
        {
            type = Set_Sframe_Key;
            config_type_len = strlen(set_sframe_key);
        }
        // TODO these need more work
        // else if (strcmp((const char*)config_buff, (const char*)get_ssid_0) == 0)
        // {
        //     type = Get_Ssid_0;
        //     config_type_len = strlen(get_ssid_0);
        // }
        // else if (strcmp((const char*)config_buff, (const char*)get_ssid_1) == 0)
        // {
        //     type = Get_Ssid_1;
        //     config_type_len = strlen(get_ssid_1);
        // }
        // else if (strcmp((const char*)config_buff, (const char*)get_ssid_2) == 0)
        // {
        //     type = Get_Ssid_2;
        //     config_type_len = strlen(get_ssid_2);
        // }
        // else if (strcmp((const char*)config_buff, (const char*)get_moq_url) == 0)
        // {
        //     type = Get_Moq_Url;
        //     config_type_len = strlen(get_moq_url);
        // }
        else if (strcmp((const char*)config_buff, (const char*)clear_configuration) == 0)
        {
            type = Clear_Configuration;
            config_type_len = strlen(clear_configuration);

            HAL_UART_Transmit(stream->rx.uart, (const uint8_t*)"OK!\n", 4, HAL_MAX_DELAY);

            // Send the configuration to the ui chip in blocking mode.
            HAL_UART_Transmit(stream->tx.uart, (uint8_t*)&type, 1, HAL_MAX_DELAY);

            // Transmit dummy data for confirmation.
            len = 1;
            HAL_UART_Transmit(stream->tx.uart, (uint8_t*)&len, sizeof(len), HAL_MAX_DELAY);

            HAL_UART_Transmit(stream->tx.uart, (uint8_t*)"1", 1, HAL_MAX_DELAY);
            goto cleanup;
        }

        if (type == Config_Type_Not_Found)
        {
            HAL_UART_Transmit(stream->rx.uart,
                              (const uint8_t*)"ERROR! Invalid configuration type\n", 35,
                              HAL_MAX_DELAY);
            goto cleanup;
        }

        if (next_word == NULL)
        {
            // TODO send back a NACK of sort sort to the USB
            HAL_UART_Transmit(stream->rx.uart,
                              (const uint8_t*)"ERROR! Missing argument after command\n", 38,
                              HAL_MAX_DELAY);
            ;
            goto cleanup;
        }

        // Skip the space
        for (len = config_type_len + 1; len < CONFIGURATOR_BUFF_SZ; ++len)
        {
            if (config_buff[len] == '\0')
            {
                break;
            }
        }

        if (config_buff[len] != '\0')
        {
            HAL_UART_Transmit(stream->rx.uart,
                              (const uint8_t*)"ERROR! missing null termination for argument\n", 45,
                              HAL_MAX_DELAY);
            goto cleanup;
        }

        len -= (config_type_len + 1);

        if (len == 0)
        {
            HAL_UART_Transmit(stream->rx.uart, (const uint8_t*)"ERROR! configration is too short\n",
                              33, HAL_MAX_DELAY);
            goto cleanup;
        }

        HAL_UART_Transmit(stream->rx.uart, (const uint8_t*)"OK!\n", 4, HAL_MAX_DELAY);

        // Send the configuration to the ui chip in blocking mode.
        HAL_UART_Transmit(stream->tx.uart, (uint8_t*)&type, 1, HAL_MAX_DELAY);
        HAL_UART_Transmit(stream->tx.uart, (uint8_t*)&len, sizeof(len), HAL_MAX_DELAY);
        // skip the space +1
        HAL_UART_Transmit(stream->tx.uart, (const uint8_t*)(next_word + 1), len, HAL_MAX_DELAY);

    cleanup:
        // Clean up our buff
        for (uint16_t i = 0; i < idx; ++i)
        {
            config_buff[i] = 0;
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
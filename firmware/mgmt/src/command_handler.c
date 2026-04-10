#include "command_handler.h"
#include "app_mgmt.h"
#include "chip_control.h"
#include "io_control.h"
#include "state.h"
#include "uart_router.h"

#define HELLO_REPLY_LEN COMMAND_LEN + LEN_LEN + LINK_SYNC_WORD_LEN

void command_handle_packet(const CtlToMgmt command, const uint8_t* data, const uint32_t len)
{
    switch (command)
    {
    case Ping:
    {
        command_pong();
        break;
    }
    case ToUi:
    {
        command_to_ui();
        break;
    }
    case ToNet:
    {
        command_to_net();
        break;
    }
    case HelloRequest:
    {
        command_hello_request(data, len);
        break;
    }
    case SetPin:
    {
        command_set_pin(data, len);
        break;
    }
    case SetUiBaudrate:
    {
        command_set_ui_baudrate(data, len);
        break;
    }
    default:
    {
    }
    }
}

void command_pong()
{
}

void command_to_ui()
{
}

void command_to_net()
{
}

void command_hello_request(const uint8_t* data, const uint32_t len)
{
    const uint32_t reply_len = LINK_SYNC_WORD_LEN;

    uint8_t packet[HELLO_REPLY_LEN] = {0};
    if (len != LINK_SYNC_WORD_LEN)
    {
        return;
    }

    // Should be in write to packet function
    packet[0] = HelloReply & 0xFF;
    packet[1] = HelloReply >> 8;

    packet[2] = reply_len & 0xFF;
    packet[3] = reply_len >> 8;
    packet[4] = reply_len >> 16;
    packet[5] = reply_len >> 24;

    const uint32_t data_offset = 6;
    for (uint32_t i = 0; i < len; ++i)
    {
        packet[data_offset + i] = data[i] ^ Link_Sync_Word[i];
    }

    uart_router_copy_to_tx(&uart_router_get_usb_stream()->tx, Link_Sync_Word, LINK_SYNC_WORD_LEN);
    uart_router_copy_to_tx(&uart_router_get_usb_stream()->tx, packet, HELLO_REPLY_LEN);
}

void command_set_pin(const uint8_t* data, const uint32_t len)
{
    if (len != 2)
    {
        return;
    }
}

void command_set_ui_baudrate(const uint8_t* data, const uint32_t len)
{
}

void command_get_version(void* arg)
{
    // TODO actually get a version
    uart_stream_t* stream = uart_router_get_usb_stream();
    uart_router_copy_string_to_tx(&stream->tx, "v1.0.0\n");
}

void command_who_are_you(void* arg)
{
    uart_stream_t* stream = uart_router_get_usb_stream();

    static const char Who_Are_You_Response[] = "HELLO, I AM A HACTAR DEVICE";
    uart_router_copy_string_to_tx(&stream->tx, Who_Are_You_Response);
}

void command_hard_reset(void* arg)
{
    app_mgmt_reset(Hard_Reset);
}

void command_reset(void* arg)
{
    command_reset_ui(arg);
    command_reset_net(arg);
}

void command_reset_ui(void* arg)
{
    chip_control_ui_normal_mode();
}

void command_reset_net(void* arg)
{
    chip_control_net_normal_mode();
}

void command_stop_ui(void* arg)
{
    chip_control_ui_hold_in_reset();
}

void command_stop_net(void* arg)
{
    chip_control_net_hold_in_reset();
}

void command_flash_ui(void* arg)
{
    chip_control_net_hold_in_reset();

    uint8_t* uploader = (uint8_t*)arg;

    uart_stream_t* usb_stream = uart_router_get_usb_stream();
    uart_stream_t* net_stream = uart_router_get_net_stream();
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    usb_stream->path = Tx_Path_Ui;
    net_stream->path = Tx_Path_None;
    ui_stream->path = Tx_Path_Usb;

    uart_router_usb_send_flash_ok();
    uart_router_usb_update_reinit(UART_WORDLENGTH_9B, UART_PARITY_EVEN);
    uart_router_start_receive(ui_stream);

    *uploader = 1;

    HAL_Delay(200);
    chip_control_ui_bootloader_mode();

    uart_router_usb_send_ready();
}

void command_flash_net(void* arg)
{
    chip_control_ui_hold_in_reset();

    uint8_t* uploader = (uint8_t*)arg;

    uart_stream_t* usb_stream = uart_router_get_usb_stream();
    uart_stream_t* net_stream = uart_router_get_net_stream();
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    usb_stream->path = Tx_Path_Net;
    net_stream->path = Tx_Path_Usb;
    ui_stream->path = Tx_Path_None;

    uart_router_usb_send_flash_ok();
    uart_router_usb_update_reinit(UART_WORDLENGTH_8B, UART_PARITY_NONE);
    uart_router_start_receive(net_stream);

    *uploader = 1;

    HAL_Delay(200);
    chip_control_net_bootloader_mode();

    uart_router_usb_send_ready();
}

void command_enable_logs(void* arg)
{
    command_enable_logs_ui(arg);
    command_enable_logs_net(arg);
}

void command_enable_logs_ui(void* arg)
{
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    ui_stream->path = Tx_Path_Usb;
}

void command_enable_logs_net(void* arg)
{
    uart_stream_t* net_stream = uart_router_get_net_stream();

    net_stream->path = Tx_Path_Usb;
}

void command_disable_logs(void* arg)
{
    command_disable_logs_ui(arg);
    command_disable_logs_net(arg);
}

void command_disable_logs_ui(void* arg)
{
    uart_stream_t* ui_stream = uart_router_get_ui_stream();

    ui_stream->path = Tx_Path_None;
}

void command_disable_logs_net(void* arg)
{
    uart_stream_t* net_stream = uart_router_get_net_stream();

    net_stream->path = Tx_Path_None;
}

void command_default_logging(void* arg)
{
    const State* state = (const State*)arg;
    uart_stream_t* ui_stream = uart_router_get_ui_stream();
    uart_stream_t* net_stream = uart_router_get_net_stream();

    switch (*state)
    {
    case Normal:
        net_stream->path = Tx_Path_None;
        ui_stream->path = Tx_Path_None;
        break;
    case Debug:
        net_stream->path = Tx_Path_Usb;
        ui_stream->path = Tx_Path_Usb;
        break;
    default:
        break;
    }
}

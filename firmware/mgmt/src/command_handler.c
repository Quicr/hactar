#include "command_handler.h"
#include "app_mgmt.h"
#include "chip_control.h"
#include "io_control.h"
#include "uart_router.h"

void command_get_version(void* arg)
{
    uart_stream_t* stream = uart_router_get_usb_stream();
    uart_router_copy_string_to_tx(&stream->tx, "v1.0.0");
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
    uart_router_usb_reinit(UART_WORDLENGTH_9B, UART_PARITY_EVEN);
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
    uart_router_usb_reinit(UART_WORDLENGTH_8B, UART_PARITY_NONE);
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

void command_default_logs(void* arg)
{
    // TODO
}
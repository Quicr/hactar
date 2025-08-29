#include "command_handler.h"
#include "app_mgmt.h"
#include "chip_control.h"
#include "io_control.h"
#include "uart_router.h"

void command_get_version(void* arg)
{
    uart_stream_t* stream = uart_router_get_usb_stream();
    uart_router_copy_string_to_tx(stream->tx, "v1.0.0");
}

void command_who_are_you(void* arg)
{
    uart_stream_t* stream = uart_router_get_usb_stream();

    static const char Who_Are_You_Response[] = "HELLO, I AM A HACTAR DEVICE";
    uart_router_copy_string_to_tx(stream->tx, Who_Are_You_Response);
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
    UINormalMode();
}

void command_reset_net(void* arg)
{
    NetNormalMode();
}

void command_flash_ui(void* arg)
{
    uint8_t* uploader = (uint8_t*)arg;

    uart_stream_t* stream = uart_router_get_usb_stream();

    uart_router_send_flash_ok();

    stream->path = Tx_Path_Ui;
    *uploader = 1;

    // uart_router_usb_reinit(UART_WORDLENGTH_9B, UART_PARITY_EVEN);

    UIBootloaderMode();

    uart_router_send_ready();
}

void command_flash_net(void* arg)
{
}

void command_toggle_logs(void* arg)
{
}
#include "command_handler.h"
#include "uart_router.h"

void command_get_version(void* arg)
{
    uart_stream_t* stream = uart_router_get_usb_stream();
    uart_router_copy_string_to_tx(stream->tx, "v1.0.0");
}
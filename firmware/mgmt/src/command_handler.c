#include "command_handler.h"
#include "app_mgmt.h"
#include "chip_control.h"
#include "io_control.h"
#include "main.h"
#include "state.h"
#include "stm32f072xb.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_gpio.h"
#include "uart_router.h"
#include <stdint.h>

#define HELLO_REPLY_LEN TYPE_LEN + LENGTH_LEN + LINK_SYNC_WORD_LEN

void command_handle_packet(const tlv_packet_t* packet)
{
    switch (packet->type)
    {
    case Ping:
    {
        command_pong();
        break;
    }
    case ToUi:
    {
        command_to_ui(packet);
        break;
    }
    case ToNet:
    {
        command_to_net(packet);
        break;
    }
    case HelloRequest:
    {
        command_hello_request(packet);
        break;
    }
    case SetPin:
    {
        command_set_pin(packet);
        break;
    }
    case SetUiBaudrate:
    {
        command_set_ui_baudrate(packet);
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

void command_to_ui(const tlv_packet_t* packet)
{
    uart_router_copy_to_tx(&uart_router_get_ui_stream()->tx, packet->value, packet->len);
}

void command_to_net(const tlv_packet_t* packet)
{
    uart_router_copy_to_tx(&uart_router_get_net_stream()->tx, packet->value, packet->len);
}

void command_hello_request(const tlv_packet_t* packet)
{
    if (packet->len != LINK_SYNC_WORD_LEN)
    {
        return;
    }

    tlv_packet_t reply;
    reply.type = HelloReply;
    reply.len = LINK_SYNC_WORD_LEN;

    for (uint32_t i = 0; i < packet->len; ++i)
    {
        reply.value[i] = packet->value[i] ^ Link_Sync_Word[i];
    }

    uart_router_write_tlv(&uart_router_get_usb_stream()->tx, &reply);
}

void command_set_pin(const tlv_packet_t* packet)
{
    if (packet->len != 2)
    {
        return;
    }

    PinId pin_id = packet->value[0];
    GPIO_PinState state = (GPIO_PinState)packet->value[1];

    switch (pin_id)
    {
    case UiBoot0:
    {
        HAL_GPIO_WritePin(UI_BOOT0_GPIO_Port, UI_BOOT0_Pin, state);
        break;
    }
    case UiBoot1:
    {
        HAL_GPIO_WritePin(UI_BOOT1_GPIO_Port, UI_BOOT1_Pin, state);
        break;
    }
    case UiRst:
    {
        HAL_GPIO_WritePin(UI_NRST_GPIO_Port, UI_NRST_Pin, state);
        break;
    }
    case NetBoot:
    {
        HAL_GPIO_WritePin(NET_BOOT_GPIO_Port, NET_BOOT_Pin, state);
        break;
    }
    case NetRst:
    {
        HAL_GPIO_WritePin(NET_NRST_GPIO_Port, NET_NRST_Pin, state);
        break;
    }
    default:
    {
        return;
    }
    }

    HAL_Delay(100);

    uart_router_reply_ack();
}

void command_set_ui_baudrate(const tlv_packet_t* packet)
{
    if (packet->len != 4)
    {
        // Should reply with a nack imo
        return;
    }

    uint32_t baudrate = packet->value[0];
    baudrate |= packet->value[1] << 8;
    baudrate |= packet->value[2] << 16;
    baudrate |= packet->value[3] << 24;

    uart_router_update_baudrate(uart_router_get_ui_stream(), baudrate);

    HAL_Delay(100);

    uart_router_reply_ack();
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

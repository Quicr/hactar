#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class MgmtLinkHandler
{

private:
    TaskHandle_t net_mgmt_serial_read_handle;
    StaticTask_t net_mgmt_serial_read_buffer;
    StackType_t* net_mgmt_serial_read_stack = nullptr;

    uint8_t net_mgmt_uart_tx_buff[NET_MGMT_UART_TX_BUFF_SIZE] = {0};
    uint8_t net_mgmt_uart_rx_buff[NET_MGMT_UART_RX_BUFF_SIZE] = {0};

    uart_config_t net_mgmt_uart_config = {
        .baud_rate = 1000000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };

    Serial mgmt_layer(UART_NUM_0,
                      UART0,
                      net_mgmt_serial_read_handle,
                      ETS_UART0_INTR_SOURCE,
                      net_mgmt_uart_config,
                      NET_MGMT_UART_TX_PIN,
                      NET_MGMT_UART_RX_PIN,
                      UART_PIN_NO_CHANGE,
                      UART_PIN_NO_CHANGE,
                      *net_mgmt_uart_tx_buff,
                      NET_MGMT_UART_TX_BUFF_SIZE,
                      *net_mgmt_uart_rx_buff,
                      NET_MGMT_UART_RX_BUFF_SIZE,
                      NET_MGMT_UART_RING_RX_NUM,
                      0,
                      256,
                      2,
                      true,
                      false);

    static bool CreateMgmtLinkPacketTask();
    static void MgmtLinkPacketTask(void* args);
};

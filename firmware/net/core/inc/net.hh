#pragma once

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "logger.hh"
#include "nvs_flash.h"
#include "serial.hh"
#include "spdlog/spdlog.h"
#include "ui_net_link.hh"
#include "wifi.hh"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <memory>

struct NetTraits
{
    struct UiUart
    {
        static constexpr uart_port_t port = UART_NUM_1;
        static constexpr int tx_pin = GPIO_NUM_17;
        static constexpr int rx_pin = GPIO_NUM_18;
        static constexpr uint32_t tx_buffer_size = 8192;
        static constexpr uint32_t rx_buffer_size = 16384;
        static constexpr uint32_t ring_tx_count = 30;
        static constexpr uint32_t ring_rx_count = 30;
        static constexpr uint32_t baud_rate = 460800;

        static constexpr uart_config_t config = {
            .baud_rate = static_cast<int>(baud_rate),
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
            .flags = {},
        };

        static uart_dev_t& Uart()
        {
            return UART1;
        }
        static uint8_t& TxBuff()
        {
            static uint8_t tx_buff[tx_buffer_size] = {0};
            return *tx_buff;
        }
        static uint8_t& RxBuff()
        {
            static uint8_t rx_buff[rx_buffer_size] = {0};
            return *rx_buff;
        }
    };

    struct MgmtUart
    {
        static constexpr uart_port_t port = UART_NUM_0;
        static constexpr gpio_num_t tx_pin = GPIO_NUM_43;
        static constexpr gpio_num_t rx_pin = GPIO_NUM_44;
        static constexpr uint32_t tx_buffer_size = 1024;
        static constexpr uint32_t rx_buffer_size = 1024;
        static constexpr uint32_t ring_tx_count = 3;
        static constexpr uint32_t ring_rx_count = 3;
        static constexpr uint32_t baud_rate = 1000000;

        static constexpr uart_config_t config = {
            .baud_rate = static_cast<int>(baud_rate),
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .source_clk = UART_SCLK_DEFAULT,
            .flags = {},

        };

        static uart_dev_t& Uart()
        {
            return UART0;
        }
        static uint8_t& TxBuff()
        {
            static uint8_t tx_buff[tx_buffer_size] = {0};
            return *tx_buff;
        }
        static uint8_t& RxBuff()
        {
            static uint8_t rx_buff[rx_buffer_size] = {0};
            return *rx_buff;
        }
    };
};

struct Blaster
{
    volatile bool enabled = false;
    TaskHandle_t task_handle = nullptr;
    uint32_t packet_size;
};

struct Diagnostics
{
    bool loopback = false;
    bool logs_disabled = false;
    spdlog::level::level_enum last_spd_log_level =
        static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);
    Blaster blaster;
};

struct Runtime
{
    uint64_t device_id;
    SemaphoreHandle_t audio_req_smpr;
    uint64_t curr_audio_isr_time;
    uint64_t last_audio_isr_time;
};

class MoqContext;

struct UiLinkTaskContext
{
    Serial& ui_layer;
    Serial& mgmt_layer;
    MoqContext& moq_context;
    Runtime& runtime;
};

bool CreateUILinkPacketTask(UiLinkTaskContext& context);

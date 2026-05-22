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
#include <memory>

struct NetTraits
{
    struct UiUart
    {
        static constexpr uart_port_t port = UART_NUM_1;
        static constexpr int tx_pin = GPIO_NUM_17;
        static constexpr int rx_pin = GPIO_NUM_18;
        static constexpr uint32_t rx_buffer_size = 16384;
        static constexpr uint32_t tx_buffer_size = 8192;
        static constexpr uint32_t ring_tx_count = 30;
        static constexpr uint32_t ring_rx_count = 30;

        static constexpr uint32_t baud_rate = 460800;
        static uart_dev_t& Uart()
        {
            return UART1;
        }
    };

    struct MgmtUart
    {
        static constexpr uart_port_t port = UART_NUM_0;
        static constexpr gpio_num_t tx_pin = GPIO_NUM_43;
        static constexpr gpio_num_t rx_pin = GPIO_NUM_44;
        static constexpr uint32_t rx_buffer_size = 1024;
        static constexpr uint32_t tx_buffer_size = 1024;

        static constexpr uint32_t ring_tx_count = 3;
        static constexpr uint32_t ring_rx_count = 3;

        static constexpr uart_config_t config = {
            .baud_rate = 1000000,
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
            static uint8_t tx_buff[NetTraits::MgmtUart::tx_buffer_size] = {0};
            return *tx_buff;
        }

        static uint8_t& RxBuff()
        {
            static uint8_t rx_buff[NetTraits::MgmtUart::rx_buffer_size] = {0};
            return *rx_buff;
        }
    };
};

struct Diagnostics
{
    bool loopback = false;
    bool logs_disabled = false;
    spdlog::level::level_enum last_spd_log_level =
        static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);
};

struct Peripherals
{
    Wifi wifi;
    Serial ui_layer;
    Serial mgmt_layer;

    // TODO leds
};

struct Runtime
{
    uint64_t device_id;
    Peripherals periphals;
    SemaphoreHandle_t audio_req_smpr;
};

// extern Wifi wifi;
// extern Serial ui_layer;
// extern std::shared_ptr<moq::Session> moq_session;
// extern SemaphoreHandle_t audio_req_smpr;

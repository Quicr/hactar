#include "net.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <iostream>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_event.h"

#include "SerialEsp.hh"
#include "SerialPacketManager.hh"
#include "NetManager.hh"

#include "Wifi.hh"
#include "NetPins.hh"

#include <qsession.h>
#include "logger.hh"

extern "C" void app_main(void)
{
    SetupPins();
    SetupComponents();

    gpio_set_level(LED_R_Pin, 1);
    gpio_set_level(LED_G_Pin, 1);
    gpio_set_level(LED_B_Pin, 1);
    gpio_set_level(NET_DBG5_Pin, 0);
    gpio_set_level(NET_DBG6_Pin, 0);

    int next = 0;
    gpio_set_level(LED_R_Pin, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT_Pin, 1);

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    while (1)
    {
        if (blink_cnt++ == 100)
        {
            gpio_set_level(LED_R_Pin, next);
            next = next ? 0 : 1;
            blink_cnt = 0;
        }

        manager->Update();

        auto state = wifi->GetState();
        if (state == Wifi::State::Connected && !qsession_connected)
        {
            Logger::Log(Logger::Level::Info, "Net app_main Connecting to QSession");
            qsession->connect();
            qsession_connected = true;
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

static void SetupPins()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_STAT_SEL;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(NET_STAT_Pin, 0);

    // LED init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = LEDS_OUTPUT_SEL;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // Debug init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_DBG_SEL;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // Configure the uart
    uart_config_t uart1_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_MAX,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };

    // Setup serial interface for ui
    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart1_config, 4096);

    Logger::Log(Logger::Level::Info, "Pin setup complete");
}

void SetupComponents()
{
    // UART to the ui
    ui_layer = new SerialPacketManager(ui_uart1);
    wifi = new Wifi(*ui_layer);

    inbound_queue = std::make_shared<AsyncQueue<QuicrObject>>();
    char default_relay [] = "192.168.50.20";
    auto relay_name = default_relay;
    uint16_t port = 1234;
    quicr::RelayInfo relay{
        .hostname = relay_name,
        .port = port,
        .proto = quicr::RelayInfo::Protocol::UDP
    };
    qsession = std::make_shared<QSession>(relay, inbound_queue);

    manager = new NetManager(*ui_layer, *wifi, qsession, inbound_queue);

    // Wait for wifi to finish initializing
    while (!wifi->IsInitialized())
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    Logger::Log(Logger::Level::Info, "Components ready");
}

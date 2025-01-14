#include "net.hh"

#include <quicr/client.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"
#include "esp_event.h"

#include "serial_esp.hh"
#include "serial_packet_manager.hh"

#include "wifi.hh"
#include "net_pins.hh"
#include "logger.hh"
#include "moq_session.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <memory>

extern "C" void app_main(void)
{

    Logger::Log(Logger::Level::Info, "Starting Net Main");

    SetupPins();

    // UART to the ui
    ui_layer = std::make_unique<SerialPacketManager>(ui_uart1.get());

    // wifi
    Wifi wifi;
    wifi.Connect("m10x-interference", "goodlife");

    while (!wifi.IsConnected())
    {
        // TODO: Remove this.
        Logger::Log(Logger::Level::Warn, "Waiting to connect to wifi");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    Logger::Log(Logger::Level::Info, "Components ready");

    PostSetup();

    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = "moq://192.168.10.246:1234";
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";

    moq::Session moq_session(config);

    Logger::Log(Logger::Level::Info, "MOQ Transport Calling Connect ");
    if (moq_session.Connect() != quicr::Transport::Status::kConnecting) {
        Logger::Log(Logger::Level::Error, "MOQ Transport Session Connection Failure");
        exit(-1);
    }

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    int next = 0;

    bool terminate = false;
    while (!terminate)
    {
        if (blink_cnt++ == 100)
        {
            gpio_set_level(NET_LED_R, next);
            next = next ? 0 : 1;
            blink_cnt = 0;
        }

        switch(moq_session.GetStatus())
        {
            case moq::Session::Status::kConnecting:
            case moq::Session::Status::kPendingSeverSetup:
                break;
            case moq::Session::Status::kReady:
                // TODO: Do stuff here
                break;
            default:
                terminate = true;
                break;
        }

        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        Logger::Log(Logger::Level::Warn, "Free Heap Size: ", free_heap);

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void SetupPins()
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_STAT_MASK;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(NET_STAT, 0);

    // LED init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = LED_MASK;
    io_conf.pull_down_en = (gpio_pulldown_t)0;
    io_conf.pull_up_en = (gpio_pullup_t)0;
    gpio_config(&io_conf);

    // Debug init
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = NET_DEBUG_MASK;
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
    ui_uart1 = std::make_unique<SerialEsp>(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart1_config, 4096);

    Logger::Log(Logger::Level::Info, "Pin setup complete");
}

void SetupComponents(const DeviceSetupConfig& config)
{

}


void PostSetup()
{
    gpio_set_level(NET_LED_R, 1);
    gpio_set_level(NET_LED_G, 1);
    gpio_set_level(NET_LED_B, 1);
    gpio_set_level(NET_DEBUG_1, 0);
    gpio_set_level(NET_DEBUG_2, 0);
    gpio_set_level(NET_DEBUG_3, 0);

    gpio_set_level(NET_LED_R, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT, 1);

}
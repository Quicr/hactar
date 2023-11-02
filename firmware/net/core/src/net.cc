
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

#include "Logging.hh"
#include "SerialLogger.hh"
#include "SerialEsp.hh"
#include "SerialManager.hh"
#include "NetManager.hh"

#include "wifi.h"
#include "NetPins.hh"

#include <qsession.h>

static const char* TAG = "[Net-Main]";

using namespace net_wifi;
std::shared_ptr<Wifi> wifi;

// Defines

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static hactar_utils::LogManager* logger;

static NetManager* manager = nullptr;
static SerialEsp* ui_uart1 = nullptr;
static SerialManager* ui_layer = nullptr;
static std::shared_ptr<QSession> qsession = nullptr;

// Forward declare functions
void Setup(const uart_config_t&);
void Run();


static void wifi_monitor() {

    auto state = wifi->get_state();
    switch (state)
    {
        case Wifi::State::ReadyToConnect:
        case Wifi::State::Disconnected: {
            logger->info(TAG, "Wifi : Ready to connect/Disconnected\n");
            wifi->connect();
        }
        default:
            break;
    }

}

extern "C" void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    wifi = std::make_shared<Wifi>();
    // Configure the uart
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_MAX,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };

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


    gpio_set_level(LED_R_Pin, 1);
    gpio_set_level(LED_G_Pin, 1);
    gpio_set_level(LED_B_Pin, 1);
    gpio_set_level(NET_DBG5_Pin, 0);
    gpio_set_level(NET_DBG6_Pin, 0);

    int next = 0;
    gpio_set_level(LED_R_Pin, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT_Pin, 1);


    // perform setup
    Setup(uart_config);

    int32_t count = 0;
    while (1)
    {
        gpio_set_level(LED_R_Pin, next);
        next = next ? 0 : 1;
        Run();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Setup(const uart_config_t& uart_config)
{
    // setup logger
    logger = hactar_utils::LogManager::GetInstance();
    logger->add_logger(new hactar_utils::ESP32SerialLogger());
    logger->info(TAG, "Net app_main start\r\n");


    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 256);
    ui_layer = new SerialManager(ui_uart1);

    esp_event_loop_create_default();
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        logger->warn(TAG, "nvs_flash_init - no free-pages/version issue ");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

     // Wifi setup
    Wifi::State wifi_state { Wifi::State::NotInitialized };
    wifi->SetCredentials("ramanujan", "JaiGanesha!23");
    wifi->init();

    manager = new NetManager(ui_layer, wifi, nullptr);

}

static bool qsession_connected = false;
void Run()
{
    static bool subscribed = false;

    wifi_monitor();
    auto state = wifi->get_state();
    if (state == Wifi::State::Connected && !qsession_connected ) {
        logger->info(TAG, "Net app_main Connecting to QSession\n");
        char default_relay [] = "10.0.0.229";
        auto relay_name = default_relay;
        uint16_t port = 33434;
        quicr::RelayInfo relay {
            .hostname = relay_name,
            .port = port,
            .proto = quicr::RelayInfo::Protocol::UDP
        };
        qsession = std::make_shared<QSession>(relay);
        qsession->connect();
        qsession_connected = true;

        logger->info(TAG, "netcc: Subscribing to namespace");
        //quicr::Namespace ns = qsession->to_namespace("quicr://webex.cisco.com/version/1/appId/1/org/1/channel/100/room/1");
        quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);
        std::cout << "Subscribing to " << nspace << std::endl;
        qsession->subscribe(nspace);
    }




}

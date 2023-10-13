/* GPIO Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
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

#include "Wifi.hh"
#include "Logging.hh"
#include "SerialLogger.hh"
#include "SerialEsp.hh"
#include "SerialManager.hh"
#include "NetManager.hh"

#include "NetPins.hh"

#include <cantina/logger.h>
#include <transport/transport.h>
#include <quicr/quicr_client.h>
#include <quicr/quicr_client_delegate.h>
#include <quicr/name.h>

static const char* TAG = "net-main";


// Defines

// #define BUFFER_SIZE (128)

// static QueueHandle_t uart1_queue;
static hactar_utils::LogManager* logger;

static NetManager* manager = nullptr;
static SerialEsp* ui_uart1 = nullptr;
static SerialManager* ui_layer = nullptr;
static quicr::QuicRClient* qclient = nullptr;
cantina::LoggerPointer qlogger =
    std::make_shared<cantina::Logger>("quicr_logger");

// quicr helper
class SubDelegate : public quicr::SubscriberDelegate
{
public:
    SubDelegate()
    {
    }

    void onSubscribeResponse(
        [[maybe_unused]] const quicr::Namespace& quicr_namespace,
        [[maybe_unused]] const quicr::SubscribeResult& result) override
    {
        printf("onsubres\n\r");
        std::cout << "onSubscriptionResponse: name: " << quicr_namespace
            << " status: " << int(static_cast<uint8_t>(result.status))
            << std::endl;

    }

    void onSubscriptionEnded(
        [[maybe_unused]] const quicr::Namespace& quicr_namespace,
        [[maybe_unused]] const quicr::SubscribeResult::SubscribeStatus& reason)
        override
    {
        printf("subended\n\r");
    }

    void onSubscribedObject([[maybe_unused]] const quicr::Name& quicr_name,
        [[maybe_unused]] uint8_t priority,
        [[maybe_unused]] uint16_t expiry_age_ms,
        [[maybe_unused]] bool use_reliable_transport,
        [[maybe_unused]] quicr::bytes&& data) override
    {
        printf("onsubobj\n\r");

        std::cout << "onSubscribedObject:  Name: " << quicr_name << " data sz: " << data.size() << std::endl;
    }

    void onSubscribedObjectFragment(
        [[maybe_unused]] const quicr::Name& quicr_name,
        [[maybe_unused]] uint8_t priority,
        [[maybe_unused]] uint16_t expiry_age_ms,
        [[maybe_unused]] bool use_reliable_transport,
        [[maybe_unused]] const uint64_t& offset,
        [[maybe_unused]] bool is_last_fragment,
        [[maybe_unused]] quicr::bytes&& data) override
    {
        printf("onsubresfrag\n\r");
    }

};
std::shared_ptr<SubDelegate> sub_delegate = nullptr;

class PubDelegate : public quicr::PublisherDelegate
{
public:
    PubDelegate()
    {
    }
    void onPublishIntentResponse(
        [[maybe_unused]] const quicr::Namespace& quicr_namespace,
        [[maybe_unused]] const quicr::PublishIntentResult& result) override
    {
        //printf("NET - Received publish for %s", quicr_namespace);
    }

};
std::shared_ptr<PubDelegate> pub_delegate = nullptr;

auto name = quicr::Name(std::string("abcd"));


// Forward declare functions
void Setup();
void Run();

extern "C" void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // vTaskDelay(10000 / portTICK_PERIOD_MS);

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

    // TODO put somewhere else?
    ui_uart1 = new SerialEsp(UART1, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, uart_config, 256);
    ui_layer = new SerialManager(ui_uart1);
    manager = new NetManager(ui_layer);

    gpio_set_level(LED_R_Pin, 1);
    gpio_set_level(LED_G_Pin, 1);
    gpio_set_level(LED_B_Pin, 1);
    gpio_set_level(NET_DBG5_Pin, 0);
    gpio_set_level(NET_DBG6_Pin, 0);

    int next = 0;
    gpio_set_level(LED_R_Pin, 0);

    // Ready for normal operations
    gpio_set_level(NET_STAT_Pin, 1);


    logger = hactar_utils::LogManager::GetInstance();
    logger->add_logger(new hactar_utils::ESP32SerialLogger());
    logger->info(TAG, "Net app_main start");
    sub_delegate = std::make_shared<SubDelegate>();
    pub_delegate = std::make_shared<PubDelegate>();

    int32_t count = 0;
    while (1)
    {
        gpio_set_level(LED_R_Pin, next);
        next = next ? 0 : 1;
        Run();

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void Setup()
{
    logger = hactar_utils::LogManager::GetInstance();
    logger->add_logger(new hactar_utils::ESP32SerialLogger());
    logger->info(TAG, "Net app_main start");
}

void Run()
{
    static bool subscribed = false;
    auto state = hactar_utils::Wifi::GetInstance()->GetState();

    if (state != hactar_utils::Wifi::State::Connected)
        return;

    /*
       Setup a QSession
        auto qSession = QSession(relay_info)
    */
    if (qclient == nullptr)
    {
        char default_relay [] = "192.168.50.19";
        auto relay_name = default_relay;
        uint16_t port = 33434;

        quicr::RelayInfo relay{
            .hostname = relay_name,
                .port = port,
                .proto = quicr::RelayInfo::Protocol::UDP
        };
        //auto qsession = new QSession(relay);
        //qclient = new quicr::QuicRClient(relay, {}, qlogger);
    }
    else
    {
        quicr::Namespace nspace(0xA11CEE00000001010007000000000000_name, 80);
        if (subscribed)
        {
            qclient->publishNamedObject(nspace.name(), 0, 10000, false, {'h', 'e', 'l', 'l', 'o', '!'});
            return;
        }

        qclient->publishIntent(pub_delegate, nspace, "origin_url", "auth_token", {});

        logger->info(TAG, "Subscribe\n");
        std::cout << "Subscribe to " << nspace << std::endl;
        quicr::SubscribeIntent intent = quicr::SubscribeIntent::immediate;
        quicr::bytes empty;
        qclient->subscribe(
            sub_delegate, nspace, intent, "origin_url", false, "auth_token", std::move(empty));
        subscribed = true;
    }
}

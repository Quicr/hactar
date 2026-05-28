#include "net.hh"
#include "config_state.hh"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_pthread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "link_packet_t.hh"
#include "logger.hh"
#include "mgmt_link_handler.hh"
#include "moq_context.hh"
#include "net_mgmt_link.h"
#include "peripherals.hh"
#include "serial.hh"
#include "storage.hh"
#include "stored_value.hh"
#include "ui_link_handler.hh"
#include "ui_net_link.hh"
#include "wifi.hh"
#include <nlohmann/json.hpp>
#include <quicr/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Runtime runtime_ctx{
    .device_id = 0,
    .audio_req_smpr = xSemaphoreCreateBinary(),
    .curr_audio_isr_time = 0,
    .last_audio_isr_time = 0,
};

// TODO remove me some day
#ifdef __has_include
#if __has_include("wifi_creds.hh")
#include "wifi_creds.hh"
#else
#warning "wifi_creds.hh not found!!"
#endif
#else
#include "wifi_creds.hh"
#endif

static void IRAM_ATTR GpioIsrRisingHandler(void* arg)
{
    int gpio_num = (int)arg;

    if (gpio_num == UI_READY)
    {
        runtime_ctx.last_audio_isr_time = runtime_ctx.curr_audio_isr_time;
        runtime_ctx.curr_audio_isr_time = esp_timer_get_time();
        xSemaphoreGiveFromISR(runtime_ctx.audio_req_smpr, NULL);
    }
}

void PrintRAM()
{
    NET_LOG_ERROR("Internal SRAM available: %d bytes",
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    NET_LOG_ERROR("PSRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void SetPThreadDefault()
{
    esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
    cfg.stack_size = 32000;
    cfg.stack_alloc_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    esp_pthread_set_cfg(&cfg);
}

extern "C" void app_main(void)
{
    SetPThreadDefault();
    PrintRAM();
    InitializeGPIO();
    IntitializeLEDs();
    InitializeUIReadyISR(GpioIsrRisingHandler);

    Diagnostics diagnostics;
    Storage storage;
    ConfigState config(storage);

    Serial ui_layer(NetTraits::UiUart::port, NetTraits::UiUart::Uart(), ETS_UART1_INTR_SOURCE,
                    NetTraits::UiUart::config, NetTraits::UiUart::tx_pin, NetTraits::UiUart::rx_pin,
                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, NetTraits::UiUart::TxBuff(),
                    NetTraits::UiUart::tx_buffer_size, NetTraits::UiUart::RxBuff(),
                    NetTraits::UiUart::rx_buffer_size, NetTraits::UiUart::ring_rx_count, 8192, 8192,
                    20, true, false);

    Serial mgmt_layer(NetTraits::MgmtUart::port, NetTraits::MgmtUart::Uart(), ETS_UART0_INTR_SOURCE,
                      NetTraits::MgmtUart::config, NetTraits::MgmtUart::tx_pin,
                      NetTraits::MgmtUart::rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                      NetTraits::MgmtUart::TxBuff(), NetTraits::MgmtUart::tx_buffer_size,
                      NetTraits::MgmtUart::RxBuff(), NetTraits::MgmtUart::rx_buffer_size,
                      NetTraits::MgmtUart::ring_rx_count, 0, 256, 2, true, false);

    Wifi wifi(storage);
    MoqContext moq_context(ui_layer, runtime_ctx, diagnostics);
    UiLinkHandler ui_link_handler(ui_layer, mgmt_layer, moq_context, runtime_ctx);
    MgmtLinkHandler mgmt_link_handler(mgmt_layer, ui_layer, wifi, storage, config, diagnostics,
                                      moq_context);

    NET_LOG_INFO("Starting Net Main");

    mgmt_link_handler.Begin();
    ui_link_handler.Begin();

    wifi.Begin();

#if defined(my_ssid) && defined(my_ssid_pwd)
    wifi.Connect(my_ssid, my_ssid_pwd);
#endif

    if (config.moq_server_url->empty())
    {
        // No moq url found, using default
        NET_LOG_WARN(
            "No moq server url found, using default moq://relay.us-west-2.quicr.ctgpoc.com:33435");
        config.moq_server_url = "moq://relay.us-west-2.quicr.ctgpoc.com:33435";
    }

    NET_LOG_WARN("Using moq server url of %s len %u", config.moq_server_url->c_str(),
                 config.moq_server_url->length());

    // Set default language if not configured
    if (config.language->empty())
    {
        config.language = "en-US";
    }

    // Load namespaces from storage
    config.LoadNamespacesFromStorage();

    // Log warnings if namespaces are not configured (no defaults - must be configured via
    // fl-identity)
    // TODO maybe we should loop forever at this point?
    if (config.channel_ns.empty())
    {
        NET_LOG_WARN("Channel namespace not configured - device needs configuration");
    }
    if (config.ai_query_ns.empty() || config.ai_audio_response_ns.empty()
        || config.ai_cmd_response_ns.empty())
    {
        NET_LOG_WARN("AI namespaces not configured - device needs configuration");
    }

    config.user_id.Load();
    if (config.user_id.stored == 0)
    {
        NET_LOG_ERROR("User id is zero");
    }

    config.user_name.Load();
    if (config.user_name->empty())
    {
        config.user_name = "unknown";
    }

    NET_LOG_INFO("User id %d user name %s", (int)config.user_id.stored, config.user_name->c_str());

    // Use mac addr as id for my session
    uint64_t mac = 0;
    esp_efuse_mac_get_default((uint8_t*)&mac);
    mac = mac << 2;
    mac = mac >> 2;
    runtime_ctx.device_id = mac;

    NET_LOG_ERROR("mac addr %llu", mac);

    // setup moq transport
    quicr::ClientConfig moq_config;
    moq_config.endpoint_id = std::to_string(runtime_ctx.device_id);
    moq_config.connect_uri = config.moq_server_url.Load();
    moq_config.transport_config.debug = true;
    moq_config.transport_config.use_reset_wait_strategy = false;
    moq_config.transport_config.time_queue_max_duration = 5000;
    moq_config.transport_config.tls_cert_filename = "";
    moq_config.transport_config.tls_key_filename = "";
    moq_config.tick_service_sleep_delay_us = 30000;

    moq_context.InitializeSession(moq_config);

    PrintRAM();

    NET_LOG_INFO("Components ready");

    // Set up initial tracks based on stored configuration
    moq_context.UpdateAITracks(config);
    moq_context.UpdateChannelTracks(config);

    moq::Session::Status prev_status = moq::Session::Status::kNotConnected;
    Wifi::State prev_wifi_state = Wifi::State::Connected;
    while (true)
    {
        moq::Session::Status status = moq_context.GetStatus();

        Wifi::State wifi_state = wifi.GetState();

        if (prev_wifi_state != wifi_state)
        {
            prev_wifi_state = wifi_state;

            switch (wifi_state)
            {
            case Wifi::State::Disconnected:
            {
                // Back to yellow
                gpio_set_level(NET_LED_R, 0);
                gpio_set_level(NET_LED_G, 0);
                gpio_set_level(NET_LED_B, 1);

                if (status == moq::Session::Status::kConnecting
                    || status == moq::Session::Status::kPendingServerSetup
                    || status == moq::Session::Status::kReady)
                {
                    moq_context.RestartSession(moq_config);
                }
                break;
            }
            case Wifi::State::Initialized:
            {
                break;
            }
            case Wifi::State::Connected:
            {
                gpio_set_level(NET_LED_R, 1);
                gpio_set_level(NET_LED_G, 0);
                break;
            }
            default:
            {
                // Do nothing.
                break;
            }
            }
        }

        if (prev_status != status && wifi.IsConnected())
        {
            NET_LOG_INFO("New moq state %d", (int)status);
            switch (status)
            {
            case moq::Session::Status::kReady:
            {
                // Create tracks from stored config (may have been set before session was ready)
                moq_context.UpdateChannelTracks(config);
                moq_context.UpdateAITracks(config);
                moq_context.StartTracks();

                break;
            }
            case moq::Session::Status::kNotConnected:
                [[fallthrough]];
            case moq::Session::Status::kFailedToConnect:
            {
                moq_context.RestartSession(moq_config);
                break;
            }
            case moq::Session::Status::kNotReady:
            {
                NET_LOG_INFO("MOQ Transport Calling Connect");

                if (!moq_context.Connect())
                {
                    NET_LOG_ERROR("MOQ Transport Session Connection Failure");
                }

                break;
            }
            default:
            {
                // TODO the rest
                NET_LOG_INFO("No handler for this moq state");
                break;
            }
            }
            prev_status = status;
        }

        if (moq_config.connect_uri != config.moq_server_url)
        {
            NET_LOG_INFO("Load moq server uri");
            moq_config.connect_uri = config.moq_server_url;
            NET_LOG_INFO("Make a new session with uri %s", moq_config.connect_uri.c_str());
            moq_context.RestartSession(moq_config);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

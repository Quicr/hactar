#include "net.hh"

#include "sdkconfig.h"

#include <quicr/client.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "nvs_flash.h"
#include "esp_heap_caps.h"
#include "esp_event.h"
#include "esp_mac.h"

#include "ui_net_link.hh"
#include "peripherals.hh"
#include "serial.hh"
#include "wifi.hh"
#include "logger.hh"
#include "utils.hh"
#include "chunk.hh"
#include "esp_pthread.h"
#include "macros.hh"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <memory>

#include "wifi_connect.hh"
#include "default_json.hh"


#include <nlohmann/json.hpp>

using json = nlohmann::json;

/** EXTERNAL VARIABLES */
// External variables defined in net.hh
uint64_t device_id = 0;
bool loopback = false;

std::shared_ptr<moq::Session> moq_session;
SemaphoreHandle_t audio_req_smpr = xSemaphoreCreateBinary();

/** END EXTERNAL VARIABLES */

constexpr const char* moq_server = "moq://relay.us-west-2.quicr.ctgpoc.com:33435";
// constexpr const char* moq_server = "moq://relay.us-east-2.quicr.ctgpoc.com:33435";
// constexpr const char* moq_server = "moq://192.168.50.19:33435";

TaskHandle_t serial_read_handle;
StaticTask_t serial_read_buffer;
StackType_t* serial_read_stack = nullptr;

uint8_t net_ui_uart_tx_buff[NET_UI_UART_TX_BUFF_SIZE] = { 0 };
uint8_t net_ui_uart_rx_buff[NET_UI_UART_RX_BUFF_SIZE] = { 0 };

uart_config_t net_ui_uart_config = {
    .baud_rate = 460800,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_2,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
};

Serial ui_layer(NET_UI_UART_PORT, NET_UI_UART_DEV,
    serial_read_handle, ETS_UART1_INTR_SOURCE,
    net_ui_uart_config,
    NET_UI_UART_TX_PIN, NET_UI_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
    *net_ui_uart_tx_buff, NET_UI_UART_TX_BUFF_SIZE,
    *net_ui_uart_rx_buff, NET_UI_UART_RX_BUFF_SIZE,
    NET_UI_UART_RING_RX_NUM
);

Wifi wifi;

uint64_t curr_audio_isr_time = esp_timer_get_time();
uint64_t last_audio_isr_time = esp_timer_get_time();

// TODO remove me some day
#ifdef __has_include
#   if __has_include("wifi_creds.hh")
#       include "wifi_creds.hh"
#   else
#       warning "wifi_creds.hh not found!!
#   endif
#else
#   include "wifi_creds.hh"
#endif

static void IRAM_ATTR GpioIsrRisingHandler(void* arg)
{
    int gpio_num = (int)arg;

    if (gpio_num == NET_STAT)
    {
        last_audio_isr_time = curr_audio_isr_time;
        curr_audio_isr_time = esp_timer_get_time();
        xSemaphoreGiveFromISR(audio_req_smpr, NULL);
    }
}

uint32_t request_id = 0;

static void LinkPacketTask(void* args)
{
    NET_LOG_INFO("Start link packet task");
    bool talk_stopped = false;
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (auto packet = ui_layer.Read())
        {
            switch ((ui_net_link::Packet_Type)packet->type)
            {
                case ui_net_link::Packet_Type::GetAudioLinkPacket:
                {
                    // TODO NOTE might be a dead type
                    break;
                }
                case ui_net_link::Packet_Type::MoQChangeNamespace:
                {
                    // TODO check if the channel is the same and if it is don't change it.
                    // NET_LOG_INFO("got change packet");
                    // ui_net_link::ChangeNamespace change_namespace;
                    // ui_net_link::Deserialize(*packet, change_namespace);
                    // track_location = std::string(change_namespace.trackname, change_namespace.trackname_len);

                    // xSemaphoreGive(pub_change_smpr);
                    // xSemaphoreGive(sub_change_smpr);
                }
                case ui_net_link::Packet_Type::TalkStart:
                    break;
                case ui_net_link::Packet_Type::TalkStop:
                    talk_stopped = true;
                    break;
                case ui_net_link::Packet_Type::PttAIObject:
                {
                    // Channel id
                    uint32_t ext_bytes = 1;
                    uint32_t length = packet->length;

                    uint8_t channel_id = packet->payload[0];

                    // Remove the bytes already read from the payload length
                    length -= ext_bytes;

                    // NET_LOG_INFO("ptt ai chid %d", (int)channel_id);
                    // If the publisher is not ready just ignore the link packet
                    std::shared_ptr<moq::TrackWriter> writer = moq_session->Writer(channel_id);
                    writer->PushPttAIObject(packet->payload + 1, length, talk_stopped,
                        curr_audio_isr_time, request_id);
                    talk_stopped = false;

                    break;
                }
                case ui_net_link::Packet_Type::PttMultiObject:
                    [[fallthrough]];
                case ui_net_link::Packet_Type::PttObject:
                {
                    // In the future I would want to use the audio object to transmit
                    // that to the relay? and do less copying but thats asking a lot.
                    // NET_LOG_INFO("serial recv audio");

                    // Channel id
                    uint32_t ext_bytes = 1;
                    uint32_t length = packet->length;

                    uint8_t channel_id = packet->payload[0];

                    // Remove the bytes already read from the payload length
                    length -= ext_bytes;


                    // NET_LOG_INFO("chid %d", (int)channel_id);
                    // If the publisher is not ready just ignore the link packet
                    std::shared_ptr<moq::TrackWriter> writer = moq_session->Writer(channel_id);
                    writer->PushPttObject(packet->payload + 1, length, talk_stopped, curr_audio_isr_time);
                    talk_stopped = false;

                    // TODO use notifies, currently it doesn't notify fast enough?
                    // xTaskNotifyGive(rtos_pub_handle);

                    break;
                }
                default:
                    NET_LOG_ERROR("Got a packet without a handler %d", (int)packet->type);

                    // for (int i = 0 ; i < NET_UI_UART_RX_BUFF_SIZE; ++i)
                    // {
                    //     NET_LOG_INFO("idx %d: %d", i, (int)net_ui_uart_rx_buff[i]);
                    // }
                    // abort();

                    break;
            }
        }
    }
}

void PrintRAM()
{
    NET_LOG_ERROR("Internal SRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
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

    NET_LOG_INFO("Starting Net Main");

    gpio_config_t io_conf = {
        .pin_bit_mask = NET_STAT_MASK,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(NET_STAT, GpioIsrRisingHandler, (void*)NET_STAT);

    InitializeGPIO();
    IntitializeLEDs();

    wifi.Begin();

    wifi.Connect("m10x-interference", "goodlife");

    #if defined(my_ssid) && defined(my_ssid_pwd)
    wifi.Connect(my_ssid, my_ssid_pwd);
    #endif

    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = moq_server;
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";
    config.tick_service_sleep_delay_us = 30000;

    // Use mac addr as id for my session
    uint64_t mac = 0;
    esp_efuse_mac_get_default((uint8_t*)&mac);
    mac = mac >> 2;
    mac = mac << 2;
    device_id = mac;

    NET_LOG_ERROR("mac addr %llu", mac);

    moq_session.reset(new moq::Session(config));

    PrintRAM();

    NET_LOG_INFO("Components ready");

    json subscriptions = default_channel_json.at("subscriptions");
    for (int i = 0; i < 2; ++i)
    {
        // NOTE- I am not doing all of the subs because I don't want text rn
        moq_session->StartReadTrack(subscriptions[i], ui_layer);
    }

    json publications = default_channel_json.at("publications");
    for (int i = 0; i < publications.size(); ++i)
    {
        moq_session->StartWriteTrack(publications[i]);
    }

    CreateLinkPacketTask();

    int next = 0;
    int64_t heartbeat = 0;
    bool ready_to_connect_moq = false;
    moq::Session::Status prev_status = moq::Session::Status::kReady;
    Wifi::State prev_wifi_state = Wifi::State::Connected;
    while (true)
    {
        moq::Session::Status status = moq_session->GetStatus();
        Wifi::State wifi_state = wifi.GetState();

        // TODO cleanup
        if (prev_wifi_state != wifi_state)
        {
            prev_wifi_state = wifi_state;
            gpio_set_level(NET_LED_G, 1);
            switch (wifi_state)
            {
                case Wifi::State::Disconnected:
                {
                    if (status == moq::Session::Status::kConnecting ||
                        status == moq::Session::Status::kPendingSeverSetup ||
                        status == moq::Session::Status::kReady)
                    {
                        moq_session->Disconnect();
                    }
                }
                case Wifi::State::Initialized:
                {
                    break;
                }
                case Wifi::State::Connected:
                {
                    // TODO send a serial message saying we are
                    // connected to wifi
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

        // TODO move into a different task?
        if (prev_status != status && wifi.IsConnected())
        {
            switch (status)
            {
                case moq::Session::Status::kReady:
                {
                    // TODO
                    // Tell ui chip we are ready
                    gpio_set_level(NET_LED_B, 0);
                    break;
                }
                case moq::Session::Status::kNotReady:
                case moq::Session::Status::kNotConnected:
                case moq::Session::Status::kFailedToConnect:
                {
                    NET_LOG_INFO("MOQ Transport Calling Connect");

                    if (moq_session->Connect() != quicr::Transport::Status::kConnecting)
                    {
                        NET_LOG_ERROR("MOQ Transport Session Connection Failure");
                    }
                    gpio_set_level(NET_LED_B, 1);

                    break;
                }
                default:
                {
                    // TODO the rest
                    break;
                }
            }
            prev_status = status;
        }

        // if (esp_timer_get_time_ms() > heartbeat)
        // {
        //     // NET_LOG_INFO("time %lld", esp_timer_get_time_ms());
        //     gpio_set_level(NET_LED_G, next);
        //     next = next ? 0 : 1;
        //     heartbeat = esp_timer_get_time_ms() + 1000;
        // }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void SetupComponents(const DeviceSetupConfig& config)
{}

bool CreateLinkPacketTask()
{
    constexpr size_t stack_size = 4096;
    serial_read_stack = (StackType_t*)heap_caps_malloc(stack_size * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
    if (serial_read_stack == NULL)
    {
        NET_LOG_INFO("Failed to allocate stack for link packet handler");
        return false;
    }
    serial_read_handle = xTaskCreateStatic(LinkPacketTask, "link packet handler", stack_size, NULL, 10, serial_read_stack, &serial_read_buffer);

    NET_LOG_INFO("Created link packet handler PSRAM left %ld", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return true;
}
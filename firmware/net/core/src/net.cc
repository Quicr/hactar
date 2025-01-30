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

#include "ui_net_link.hh"
#include "peripherals.hh"
#include "serial.hh"
#include "wifi.hh"
#include "logger.hh"
#include "moq_session.hh"
#include "utils.hh"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <memory>

#include "wifi_connect.hh"

#define NET_UI_UART_PORT UART_NUM_1
#define NET_UI_UART_DEV UART1
#define NET_UI_UART_TX_PIN 17
#define NET_UI_UART_RX_PIN 18
#define NET_UI_UART_RX_BUFF_SIZE 1024
#define NET_UI_UART_TX_BUFF_SIZE 1024
#define NET_UI_UART_RING_TX_NUM 30
#define NET_UI_UART_RING_RX_NUM 30


int num_audio_requests = 0;
int num_sent_link_audio = 0;


uart_config_t net_ui_uart_config = {
    .baud_rate = 921600,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_EVEN,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
};

Serial ui_layer(NET_UI_UART_PORT, NET_UI_UART_DEV, ETS_UART1_INTR_SOURCE,
    net_ui_uart_config,
    NET_UI_UART_TX_PIN, NET_UI_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
    NET_UI_UART_RX_BUFF_SIZE, NET_UI_UART_TX_BUFF_SIZE,
    NET_UI_UART_RING_RX_NUM, NET_UI_UART_RING_TX_NUM);

uint64_t group_id{ 0 };
uint64_t object_id{ 0 };
uint64_t subgroup_id{ 0 };

struct link_data_obj
{
    quicr::ObjectHeaders headers = {
        group_id,
        object_id,
        subgroup_id,
        0,
        quicr::ObjectStatus::kAvailable,
        2 /*priority*/,
        3000 /* ttl */,
        std::nullopt,
        std::nullopt,
    };
    std::vector<uint8_t> data;
};

std::mutex object_mux;
std::mutex sub_pub_mux;
std::deque<link_data_obj> moq_objects;

std::shared_ptr<moq::Session> moq_session;
std::shared_ptr<moq::TrackWriter> pub_track_handler;
std::shared_ptr<moq::AudioTrackReader> sub_track_handler;


static void LinkPacketTask(void* args)
{
    NET_LOG_INFO("Start link packet task");
    while (true)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        auto packet = ui_layer.Read();
        if (packet == nullptr)
        {
            continue;
        }

        switch ((ui_net_link::Packet_Type)packet->type)
        {
            case ui_net_link::Packet_Type::GetAudioLinkPacket:
            {
                // NET_LOG_INFO("recvreq");
                ++num_audio_requests;
                break;
            }
            case ui_net_link::Packet_Type::TalkStart:
                break;
            case ui_net_link::Packet_Type::TalkStop:
                break;
            case ui_net_link::Packet_Type::AudioMultiObject:
                [[fallthrough]];
            case ui_net_link::Packet_Type::AudioObject:
            {
                // NET_LOG_INFO("serial recv audio");
                link_data_obj obj;
                obj.data = std::move(std::vector<uint8_t>(packet->payload, packet->payload + packet->length));
                obj.headers.object_id++;
                obj.headers.payload_length = obj.data.size();
                std::lock_guard<std::mutex> lock(object_mux);
                moq_objects.push_back(std::move(obj));

                break;
            }
            default:
                break;
        }
    }
}

static void MoqPubTask(void* args)
{
    NET_LOG_INFO("Start publish task");

    // Make scope so that lock is released and mem reclaimed
    {
        pub_track_handler.reset(new moq::TrackWriter(moq::MakeFullTrackName("hactar-audio", "test", 1001), quicr::TrackMode::kDatagram, 2, 100));
        moq_session->PublishTrack(pub_track_handler);
        NET_LOG_INFO("Started publisher");

        while (pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk)
        {
            NET_LOG_INFO("Waiting for pub ok!");
            vTaskDelay(300 / portTICK_PERIOD_MS);
            continue;
        }

        NET_LOG_INFO("Publishing!");
    }

    while (moq_session && moq_session->GetStatus() == moq::Session::Status::kReady)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (pub_track_handler && pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk)
        {
            continue;
        }

        if (moq_objects.size() > 0)
        {
            // NET_LOG_INFO("pub audio");

            std::lock_guard<std::mutex> lock(object_mux);
            link_data_obj obj = std::move(moq_objects.front());
            moq_objects.pop_front();
            pub_track_handler->PublishObject(obj.headers, obj.data);
        }
    }

    NET_LOG_INFO("Delete pub task");
    vTaskDelete(NULL);
}

static void MoqSubTask(void* args)
{
    NET_LOG_INFO("Start subscribe task");

    // Make a scope so the memory for lock is reclaimed
    {
        sub_track_handler.reset(new moq::AudioTrackReader(moq::MakeFullTrackName("hactar-audio", "test", 2001), 10));
        moq_session->SubscribeTrack(sub_track_handler);
        NET_LOG_INFO("Started subscriber");

        while (sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk)
        {
            NET_LOG_INFO("Waiting for sub ok!");
            vTaskDelay(300 / portTICK_PERIOD_MS);
            continue;
        }

        NET_LOG_INFO("Subscribed");
    }

    while (moq_session && moq_session->GetStatus() == moq::Session::Status::kReady)
    {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk)
        {
            // TODO handling
            continue;
        }

        sub_track_handler->TryPlay();

        while (num_audio_requests > 0)
        {
            auto data = sub_track_handler->PopFront();
            if (!data.has_value())
            {
                break;
            }

            // NET_LOG_INFO("sub audio");

            link_packet_t link_packet;
            link_packet.type = static_cast<uint8_t>(ui_net_link::Packet_Type::AudioObject);
            link_packet.length = data->size();
            std::memcpy(link_packet.payload, data->data(), data->size());
            link_packet.is_ready = true;
            ui_layer.Write(&link_packet);

            --num_audio_requests;
        }

        sub_track_handler->Pause();

    }

    NET_LOG_INFO("Delete sub task");
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    NET_LOG_ERROR("Internal SRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    NET_LOG_ERROR("PSRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    NET_LOG_INFO("Starting Net Main");

    InitializeGPIO();
    IntitializeLEDs();

    // wifi
    Wifi wifi;
    // REMOVEME This is so that creds stop getting pushed
    ConnectToWifi(wifi);

    while (!wifi.IsConnected())
    {
        NET_LOG_WARN("Waiting to connect to wifi");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // FIXME: Avoids possible Brownout
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    NET_LOG_INFO("Components ready");


    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = "moq://192.168.50.20:1234";
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";

    moq_session.reset(new moq::Session(config));

    NET_LOG_INFO("MOQ Transport Calling Connect");
    if (moq_session->Connect() != quicr::Transport::Status::kConnecting)
    {
        NET_LOG_ERROR("MOQ Transport Session Connection Failure");

        // TODO: Don't exit, retry connection
        exit(-1);
    }

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    int next = 0;

    while (moq_session->GetStatus() != moq::Session::Status::kReady)
    {
        const auto moq_session_status = moq_session->GetStatus();
        if (!(moq_session_status == moq::Session::Status::kConnecting || moq_session_status == moq::Session::Status::kPendingSeverSetup))
        {
            break;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if (moq_session->GetStatus() != moq::Session::Status::kReady)
    {
        return;
    }

    NET_LOG_INFO("Moq session status %d", (int)moq_session->GetStatus());

    // Start moq tasks here
    xTaskCreate(MoqPubTask, "moq publish task", 8192, NULL, 3, NULL);
    xTaskCreate(MoqSubTask, "moq subscribe task", 8192, NULL, 2, NULL);
    xTaskCreate(LinkPacketTask, "link packet handler", 4096, NULL, 0, NULL);

    gpio_set_level(NET_STAT, 1);

    while (true)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // FIXME: Use esp_timer instead.
        // NOTE- 100 * 10ms = 1000ms :)
        if (blink_cnt++ == 1)
        {
            gpio_set_level(NET_LED_G, next);
            next = next ? 0 : 1;
            blink_cnt = 0;
        }


    }
}

void SetupComponents(const DeviceSetupConfig& config)
{
}

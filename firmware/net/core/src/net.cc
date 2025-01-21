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

extern "C" void app_main(void)
{

    Logger::Log(Logger::Level::Info, "Starting Net Main");

    InitializeGPIO();
    IntitializeLEDs();

    // UART to the ui

    uart_config_t uart1_config = {
        .baud_rate = 921600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
    };
    QueueHandle_t uart_queue;
    ui_layer = InitializeQueuedUART(uart1_config, UART1, uart_queue,
                                    RX_BUFF_SIZE, TX_BUFF_SIZE,
                                    EVENT_QUEUE_SIZE, TX_PIN, RX_PIN,
                                    RTS_PIN, CTS_PIN, ESP_INTR_FLAG_LOWMED);

    // wifi
    Wifi wifi;
    wifi.Connect("m10x-interference", "goodlife");

    while (!wifi.IsConnected())
    {
        Logger::Log(Logger::Level::Warn, "Waiting to connect to wifi");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // FIXME: Avoids possible Brownout
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    Logger::Log(Logger::Level::Info, "Components ready");

    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = "moq://192.168.10.236:1234";
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";

    moq::Session moq_session(config);

    Logger::Log(Logger::Level::Info, "MOQ Transport Calling Connect ");
    if (moq_session.Connect() != quicr::Transport::Status::kConnecting)
    {
        Logger::Log(Logger::Level::Error, "MOQ Transport Session Connection Failure");
        exit(-1);
    }

    // This is the lazy way of doing it, otherwise we should use a esp_timer.
    uint32_t blink_cnt = 0;
    int next = 0;

    int num_packets_requested = 0;

    std::shared_ptr<moq::TrackWriter> pub_track_handler;
    std::shared_ptr<moq::AudioTrackReader> sub_track_handler;

    uint64_t group_id{0};
    uint64_t object_id{0};
    uint64_t subgroup_id{0};

    while (moq_session.GetStatus() != moq::Session::Status::kReady)
    {
        const auto moq_session_status = moq_session.GetStatus();
        if (!(moq_session_status == moq::Session::Status::kConnecting || moq_session_status == moq::Session::Status::kPendingSeverSetup))
        {
            break;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    if (moq_session.GetStatus() != moq::Session::Status::kReady)
    {
        return;
    }

    if (!pub_track_handler)
    {
        pub_track_handler = std::make_shared<moq::TrackWriter>(moq::MakeFullTrackName("hactar-audio", "test", 1001), quicr::TrackMode::kDatagram, 2, 100);
        moq_session.PublishTrack(pub_track_handler);
        Logger::Log(Logger::Level::Info, "Started publisher");
    }

    if (!sub_track_handler)
    {
        sub_track_handler = std::make_shared<moq::AudioTrackReader>(moq::MakeFullTrackName("hactar-audio", "test", 2001), 1);
        moq_session.SubscribeTrack(sub_track_handler);
        Logger::Log(Logger::Level::Info, "Started subscriber");
    }

    while (true)
    {
        defer(vTaskDelay(100 / portTICK_PERIOD_MS));

        if (blink_cnt++ == 100)
        {
            gpio_set_level(NET_LED_R, next);
            next = next ? 0 : 1;
            blink_cnt = 0;
        }

        if (pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk || sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk)
        {
            continue;
        }

        sub_track_handler->Play();

        while (auto packet = ui_layer->Read())
        {
            switch ((ui_net_link::Packet_Type)packet->type)
            {
            case ui_net_link::Packet_Type::GetAudioLinkPacket:
            {
                Logger::Log(Logger::Level::Warn, "Received request for audio");
                auto data = sub_track_handler->PopFront();
                if (!data.has_value())
                {
                    continue;
                }
                Logger::Log(Logger::Level::Warn, "Responding to request for audio");


                auto link_packet = ui_layer->Write();
                link_packet->length = data->size();
                std::memcpy(link_packet->payload, data->data(), data->size());
                link_packet->is_ready = true;
                break;
            }
            case ui_net_link::Packet_Type::TalkStart:
                [[fallthrough]];
            case ui_net_link::Packet_Type::TalkStop:
                [[fallthrough]];
            case ui_net_link::Packet_Type::AudioObject:
            {
                Logger::Log(Logger::Level::Warn, "Received audio, forwarding to relay");
                
                std::vector<uint8_t> data(packet->payload, packet->payload + packet->length);
                quicr::ObjectHeaders obj_headers = {
                    group_id,
                    object_id++,
                    subgroup_id,
                    data.size(),
                    quicr::ObjectStatus::kAvailable,
                    2 /*priority*/,
                    3000 /* ttl */,
                    std::nullopt,
                    std::nullopt,
                };

                pub_track_handler->PublishObject(obj_headers, data);
                break;
            }
            default:
                break;
            }
        }

        sub_track_handler->Pause();
    }
}

void SetupComponents(const DeviceSetupConfig &config)
{
}

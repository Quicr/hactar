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
#define NET_UI_UART_RX_BUFF_SIZE 8192
#define NET_UI_UART_TX_BUFF_SIZE 8192
#define NET_UI_UART_RING_TX_NUM 30
#define NET_UI_UART_RING_RX_NUM 30

#define esp_timer_get_time_ms() (esp_timer_get_time() / 1000)


// constexpr const char* moq_server = "moq://relay.quicr.ctgpoc.com:33437";
constexpr const char* moq_server = "moq://192.168.50.19:33435";

std::string track_location = "store";
std::string base_track_namespace = "ptt.arpa/v1/org1/acme";

std::string pub_track = "pcm_en_8khz_mono_i16";
std::string sub_track = "pcm_en_8khz_mono_i16";

TaskHandle_t serial_read_handle = nullptr;
TaskHandle_t rtos_pub_handle = nullptr;
TaskHandle_t rtos_sub_handle = nullptr;
TaskHandle_t rtos_change_namespace_handle = nullptr;

uint8_t net_ui_uart_tx_buff[NET_UI_UART_TX_BUFF_SIZE] = { 0 };
uint8_t net_ui_uart_rx_buff[NET_UI_UART_RX_BUFF_SIZE] = { 0 };

// SemaphoreHandle_t num_audio_requests = xSemaphoreCreateCounting(10, 0);
int num_audio_requests = 0;

int num_sent_link_audio = 0;
int64_t last_req_time_us = 0;

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
    NET_UI_UART_RING_RX_NUM);

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

// TODO do I need semaphores?
bool change_track = true;
bool stop_pub_sub = false;
bool pub_ready = false;

SemaphoreHandle_t audio_req_smpr = xSemaphoreCreateBinary();


static void IRAM_ATTR GpioIsrRisingHandler(void* arg)
{
    int gpio_num = (int)arg;

    if (gpio_num == NET_STAT)
    {
        xSemaphoreGiveFromISR(audio_req_smpr, NULL);
    }
}


static void ChangeNamespaceTask(void* args)
{
    NET_LOG_INFO("Change namespace to %s", track_location.c_str());

    // while (true)
    // {
    //     vTaskDelay(1000);
    // }

    rtos_change_namespace_handle = NULL;
    vTaskDelete(NULL);
}

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
                    // NET_LOG_INFO("recvreq");
                    // xSemaphoreGive(num_audio_requests);
                    // last_req_time_us = esp_timer_get_time();
                    // ++num_audio_requests;
                    break;
                }
                case ui_net_link::Packet_Type::MoQChangeNamespace:
                {
                    // TODO check if the channel is the same and if it is don't change it.
                    NET_LOG_INFO("got change packet");
                    ui_net_link::ChangeNamespace change_namespace;
                    ui_net_link::Deserialize(*packet, change_namespace);
                    track_location = std::string(change_namespace.trackname, change_namespace.trackname_len);

                    // TODO use a semaphore
                    change_track = true;
                }
                case ui_net_link::Packet_Type::TalkStart:
                    break;
                case ui_net_link::Packet_Type::TalkStop:
                    talk_stopped = true;
                    break;
                case ui_net_link::Packet_Type::AudioMultiObject:
                    [[fallthrough]];
                case ui_net_link::Packet_Type::AudioObject:
                {
                    // If the publisher is not ready just ignore the link packet
                    if (!pub_ready)
                    {
                        break;
                    }
                    // NET_LOG_INFO("serial recv audio");

                    std::lock_guard<std::mutex> _(object_mux);

                    auto& obj = moq_objects.emplace_back();

                    // // Create an object
                    obj.data.resize(packet->length + 6);
                    obj.data[0] = 1;
                    obj.data[1] = 0;
                    if (talk_stopped)
                    {
                        obj.data[1] = talk_stopped;
                        talk_stopped = false;
                    }

                    uint32_t len = packet->length;
                    memcpy(obj.data.data() + 2, &len, sizeof(uint32_t));
                    memcpy(obj.data.data() + 6, packet->payload, len);

                    obj.headers.object_id++;
                    obj.headers.payload_length = len + 6;

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

static void MoqPubTask(void* args)
{
    NET_LOG_INFO("Start publish task");

    if (!moq_session)
    {
        goto delete_pub_task;
    }

    // Make scope so that lock is released and mem reclaimed
    {
        pub_track_handler.reset(
            new moq::TrackWriter(
                moq::MakeFullTrackName(base_track_namespace + track_location, pub_track, 1001),
                quicr::TrackMode::kDatagram, 2, 100)
        );

        moq_session->PublishTrack(pub_track_handler);
        NET_LOG_INFO("Started publisher");

        int64_t next_print = 0;
        while (moq_session &&
            moq_session->GetStatus() == moq::Session::Status::kReady &&
            pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk &&
            !change_track)
        {
            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("Waiting for pub ok!");
                next_print = esp_timer_get_time_ms() + 2000;
            }

            // We want to check often if we are published
            vTaskDelay(300 / portTICK_PERIOD_MS);
            continue;
        }

        NET_LOG_INFO("Publishing!");
    }

    pub_ready = true;

    while (moq_session
        && moq_session->GetStatus() == moq::Session::Status::kReady
        && !change_track)
    {
        vTaskDelay(2 / portTICK_PERIOD_MS);

        if (pub_track_handler && pub_track_handler->GetStatus() != moq::TrackWriter::Status::kOk)
        {
            continue;
        }

        if (moq_objects.size() == 0)
        {
            continue;
        }

        std::lock_guard<std::mutex> lock(object_mux);
        const link_data_obj& obj = moq_objects.front();
        pub_track_handler->PublishObject(obj.headers, obj.data);
        moq_objects.pop_front();
    }

delete_pub_task:
    NET_LOG_INFO("Delete pub task");
    pub_ready = false;
    rtos_pub_handle = NULL;
    vTaskDelete(rtos_pub_handle);
}

static void MoqSubTask(void* args)
{

    link_packet_t link_packet;
    NET_LOG_INFO("Start subscribe task");

    if (!moq_session)
    {
        NET_LOG_INFO("Start subscribe task");
        goto delete_sub_task;
    }

    // Make a scope so the memory for lock is reclaimed
    {
        sub_track_handler.reset(
            new moq::AudioTrackReader(
                moq::MakeFullTrackName(base_track_namespace + track_location, sub_track, 2001),
                10)
        );
        moq_session->SubscribeTrack(sub_track_handler);


        NET_LOG_INFO("Started subscriber");

        int64_t next_print = 0;
        while (moq_session &&
            moq_session->GetStatus() == moq::Session::Status::kReady &&
            sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk &&
            !change_track)
        {
            // if (sub_track_handler->GetStatus() != quicr::SubscribeTrackHandler::Status::kPendingResponse)
            // {
            //     NET_LOG_INFO("Sending subscribe");
            //     moq_session->SubscribeTrack(sub_track_handler);
            // }

            if (esp_timer_get_time_ms() > next_print)
            {
                NET_LOG_INFO("sub status %d", (int)sub_track_handler->GetStatus());
                NET_LOG_INFO("Waiting for sub ok!");
                next_print = esp_timer_get_time_ms() + 2000;
            }
            vTaskDelay(300 / portTICK_PERIOD_MS);
            continue;
        }

        NET_LOG_INFO("Subscribed");
    }


    while (moq_session &&
        moq_session->GetStatus() == moq::Session::Status::kReady &&
        !change_track)
    {
        try
        {
            vTaskDelay(2 / portTICK_PERIOD_MS);
            if (sub_track_handler->GetStatus() != moq::AudioTrackReader::Status::kOk)
            {
                // TODO handling
                continue;
            }

            sub_track_handler->TryPlay();

            if (xSemaphoreTake(audio_req_smpr, 0))
            {
                // TODO move?
                auto data = sub_track_handler->PopFront();
                if (!data.has_value())
                {
                    continue;
                }

                if (data->at(0) == 1)
                {
                    // Is audio

                    if (data->at(1) == 1)
                    {
                        // is last, dunno what to do with it but may need it.
                        // perhaps send some sort of link packet
                        // informing the ui that we are done?
                    }

                    uint32_t length;
                    std::memcpy(&length, &data->data()[2], sizeof(uint32_t));

                    // TODO note- this won't really work because we'd still have to wait
                    // a whole audio playout time before we can send more audio data
                    uint32_t offset = 6;
                    while (length > 0)
                    {
                        uint32_t link_packet_sz = length;
                        if (link_packet_sz > constants::Audio_Phonic_Sz + 1)
                        {
                            NET_LOG_INFO("split packet");
                            link_packet_sz = constants::Audio_Phonic_Sz + 1;
                        }

                        link_packet_t link_packet;
                        link_packet.type = static_cast<uint8_t>(ui_net_link::Packet_Type::AudioObject);
                        link_packet.length = link_packet_sz;
                        std::memcpy(link_packet.payload, data->data() + offset, link_packet_sz);
                        link_packet.is_ready = true;
                        ui_layer.Write(link_packet);

                        offset += link_packet_sz;
                        length -= link_packet_sz;
                    }

                }
                else
                {
                    // Not audio
                    // ignore?
                    NET_LOG_ERROR("Received a data chunk that is not audio");
                }
            }
        }
        catch (const std::exception& ex)
        {
            ESP_LOGE("sub", "Exception in sub %s", ex.what());
        }
    }

delete_sub_task:
    NET_LOG_INFO("Delete sub task");
    rtos_sub_handle = NULL;
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
    NET_LOG_ERROR("Internal SRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    NET_LOG_ERROR("PSRAM available: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

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

    Wifi wifi;

    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = moq_server;
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";

    moq_session.reset(new moq::Session(config));


    NET_LOG_INFO("Components ready");

    // Start moq tasks here
    xTaskCreate(LinkPacketTask, "link packet handler", 4096, NULL, 10, &serial_read_handle);

    int next = 0;
    int64_t heartbeat = 0;
    bool ready_to_connect_moq = false;
    moq::Session::Status prev_status = moq::Session::Status::kReady;
    Wifi::State prev_wifi_state = Wifi::State::Connected;
    while (true)
    {
        moq::Session::Status status = moq_session->GetStatus();
        Wifi::State wifi_state = wifi.GetState();
        if (prev_wifi_state != wifi_state)
        {
            prev_wifi_state = wifi_state;
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
                    ConnectToWifi(wifi);
                }
                case Wifi::State::Connected:
                {
                    // TODO send a serial message saying we are
                    // connected to wifi
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
            switch (status)
            {
                case moq::Session::Status::kReady:
                {
                    // TODO
                    // Tell ui chip we are ready

                    change_track = true;
                    stop_pub_sub = true;
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

        if (status == moq::Session::Status::kReady && change_track && rtos_pub_handle == nullptr && rtos_sub_handle == nullptr)
        {
            change_track = false;

            // This is put in just so the relay doesn't complain about changing to the same channel for now.
            vTaskDelay(5000 / portTICK_PERIOD_MS);

            // Wait until sub and pub tasks are cleaned up
            xTaskCreate(MoqPubTask, "moq publish task", 8192, NULL, 3, &rtos_pub_handle);
            xTaskCreate(MoqSubTask, "moq subscribe task", 8192, NULL, 2, &rtos_sub_handle);
        }

        if (esp_timer_get_time_ms() > heartbeat)
        {
            // NET_LOG_INFO("time %lld", esp_timer_get_time_ms());
            gpio_set_level(NET_LED_G, next);
            next = next ? 0 : 1;
            heartbeat = esp_timer_get_time_ms() + 1000;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void SetupComponents(const DeviceSetupConfig& config)
{}

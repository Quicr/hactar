#include "net.hh"
#include "chunk.hh"
#include "config_state.hh"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "efuse_burner.hh"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_pthread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "link_packet_t.hh"
#include "logger.hh"
#include "macros.hh"
#include "mgmt_link_handler.hh"
#include "moq_session.hh"
#include "moq_track_reader.hh"
#include "moq_track_writer.hh"
#include "net_mgmt_link.h"
#include "nvs_flash.h"
#include "peripherals.hh"
#include "picoquic_utils.h"
#include "sdkconfig.h"
#include "serial.hh"
#include "spdlog/logger.h"
#include "storage.hh"
#include "stored_value.hh"
#include "ui_net_link.hh"
#include "utils.hh"
#include "wifi.hh"
#include <inttypes.h>
#include <nlohmann/json.hpp>
#include <quicr/client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

using json = nlohmann::json;

// Forward declarations
std::shared_ptr<moq::TrackReader> CreateReadTrack(const std::string& channel_name,
                                                  const std::vector<std::string>& track_namespace,
                                                  const std::string& trackname,
                                                  const std::string& codec,
                                                  Serial& serial);
std::shared_ptr<moq::TrackWriter> CreateWriteTrack(const std::string& channel_name,
                                                   const std::vector<std::string>& track_namespace,
                                                   const std::string& trackname,
                                                   const std::string& codec);

/** EXTERNAL VARIABLES */
// External variables defined in net.hh
// uint64_t device_id = 0;

// NOTE! This can be enabled during run on via serial

std::vector<std::shared_ptr<moq::TrackReader>> readers;
std::vector<std::shared_ptr<moq::TrackWriter>> writers;

std::shared_ptr<moq::Session> moq_session;
SemaphoreHandle_t audio_req_smpr = xSemaphoreCreateBinary();

/** END EXTERNAL VARIABLES */
TaskHandle_t net_ui_serial_read_handle;
StaticTask_t net_ui_serial_read_buffer;
StackType_t* net_ui_serial_read_stack = nullptr;

uint8_t net_ui_uart_tx_buff[NetTraits::UiUart::tx_buffer_size] = {0};
uint8_t net_ui_uart_rx_buff[NetTraits::UiUart::rx_buffer_size] = {0};

uart_config_t net_ui_uart_config = {
    .baud_rate = static_cast<int>(NetTraits::UiUart::baud_rate),
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
    .flags = {},
};

Serial ui_layer(NetTraits::UiUart::port,
                NetTraits::UiUart::Uart(),
                ETS_UART1_INTR_SOURCE,
                net_ui_uart_config,
                NetTraits::UiUart::tx_pin,
                NetTraits::UiUart::rx_pin,
                UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE,
                *net_ui_uart_tx_buff,
                NetTraits::UiUart::tx_buffer_size,
                *net_ui_uart_rx_buff,
                NetTraits::UiUart::rx_buffer_size,
                NetTraits::UiUart::ring_rx_count,
                8192,
                8192,
                20,
                true,
                false);

Serial mgmt_layer(NetTraits::MgmtUart::port,
                  NetTraits::MgmtUart::Uart(),
                  ETS_UART0_INTR_SOURCE,
                  NetTraits::MgmtUart::config,
                  NetTraits::MgmtUart::tx_pin,
                  NetTraits::MgmtUart::rx_pin,
                  UART_PIN_NO_CHANGE,
                  UART_PIN_NO_CHANGE,
                  NetTraits::MgmtUart::TxBuff(),
                  NetTraits::MgmtUart::tx_buffer_size,
                  NetTraits::MgmtUart::RxBuff(),
                  NetTraits::MgmtUart::rx_buffer_size,
                  NetTraits::MgmtUart::ring_rx_count,
                  0,
                  256,
                  2,
                  true,
                  false);

uint64_t curr_audio_isr_time = esp_timer_get_time();
uint64_t last_audio_isr_time = esp_timer_get_time();

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

// TODO move into a moq handler of some sort
// Update AI tracks when AI namespaces or language changes
// AI Query: publish to {ai_query_ns, language}
// AI Audio Response: subscribe to {ai_audio_response_ns, device_id}
// AI Command Response: subscribe to {ai_cmd_response_ns, device_id}
void UpdateAITracks(Runtime& runtime, ConfigState& config)
{
    const std::string lang = config.language.Load();
    const std::string device_id_str = std::to_string(runtime.device_id);

    // AI Query publication track
    if (!config.ai_query_ns.empty() && !lang.empty())
    {
        if (auto writer = CreateWriteTrack("ai_audio", config.ai_query_ns, lang, "pcm"))
        {
            writer->Start();
        }
    }

    // AI Audio Response subscription track
    if (!config.ai_audio_response_ns.empty())
    {
        if (auto reader = CreateReadTrack("self_ai_audio", config.ai_audio_response_ns,
                                          device_id_str, "pcm", ui_layer))
        {
            reader->Start();
        }
    }

    // AI Command Response subscription track
    if (!config.ai_cmd_response_ns.empty())
    {
        if (auto reader = CreateReadTrack("self_ai_text", config.ai_cmd_response_ns, device_id_str,
                                          "ai_cmd_response:json", ui_layer))
        {
            reader->Start();
        }
    }
}

// TODO move itno a moq handler
// Update channel tracks when channel namespace or language changes
// Channel: both publish and subscribe to {channel_ns, language}
void UpdateChannelTracks(ConfigState& config)
{
    const std::string lang = config.language.Load();

    if (config.channel_ns.empty() || lang.empty())
    {
        NET_LOG_WARN("Cannot update channel tracks: namespace or language empty");
        return;
    }

    // Channel publication track
    if (auto writer = CreateWriteTrack("channel", config.channel_ns, lang, "pcm"))
    {
        writer->Start();
    }

    // Channel subscription track
    if (auto reader = CreateReadTrack("channel", config.channel_ns, lang, "pcm", ui_layer))
    {
        reader->Start();
    }
}

static void IRAM_ATTR GpioIsrRisingHandler(void* arg)
{
    int gpio_num = (int)arg;

    if (gpio_num == UI_READY)
    {
        last_audio_isr_time = curr_audio_isr_time;
        curr_audio_isr_time = esp_timer_get_time();
        xSemaphoreGiveFromISR(audio_req_smpr, NULL);
    }
}

uint32_t request_id = 0;

static void UILinkPacketTask(void* args)
{
    NET_LOG_INFO("Start ui link packet task");
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (auto packet = ui_layer.Read())
        {
            if (packet->type == static_cast<uint16_t>(ui_net_link::UiToNet::CircularPing))
            {
                // Forward to MGMT for circular path: MGMT -> UI -> NET -> MGMT
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::CircularPing),
                                 std::span<const uint8_t>(packet->payload.data(), packet->length));
                continue;
            }

            if (packet->type != static_cast<uint16_t>(ui_net_link::UiToNet::AudioFrame))
            {
                NET_LOG_ERROR("Got unexpected packet type %d", (int)packet->type);
                continue;
            }

            uint8_t channel_id = packet->payload[0];
            uint32_t ext_bytes = 1;
            uint32_t length = packet->length;

            // Remove the bytes already read from the payload length (channel_id)
            length -= ext_bytes;

            if (channel_id < (uint8_t)ui_net_link::Channel_Id::Count - 1)
            {
                if (writers[channel_id])
                {
                    writers[channel_id]->PushObject(packet->payload.data() + 1, length,
                                                    curr_audio_isr_time);
                }
            }
        }
    }
}

std::shared_ptr<moq::TrackReader> CreateReadTrack(const std::string& channel_name,
                                                  const std::vector<std::string>& track_namespace,
                                                  const std::string& trackname,
                                                  const std::string& codec,
                                                  Serial& serial)
try
{
    uint32_t offset = 0;
    if (codec == "pcm")
    {
        if (channel_name == "self_ai_audio")
        {
            offset = (uint32_t)ui_net_link::Channel_Id::Ptt_Ai;
        }
        else
        {
            offset = (uint32_t)ui_net_link::Channel_Id::Ptt;
        }
    }
    else if (codec == "ascii")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat;
    }
    else if (codec == "ai_cmd_response:json")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat_Ai;
    }
    else
    {
        // do nothing with the reader if we don't know it.
        NET_LOG_ERROR("Unknown reader channel %s", codec.c_str());
        return nullptr;
    }

    if (!moq_session)
    {
        NET_LOG_WARN("MoQ session not ready, cannot create reader");
        return nullptr;
    }

    auto desired_ftn = moq::MakeFullTrackName(track_namespace, trackname);

    std::unique_lock<std::mutex> lock = moq_session->GetReaderLock();
    if (readers[offset] != nullptr)
    {
        // If track already exists with same name, return it
        auto existing_ftn = readers[offset]->GetFullTrackName();
        if (existing_ftn.name_space == desired_ftn.name_space
            && existing_ftn.name == desired_ftn.name)
        {
            NET_LOG_INFO("Reader on %d already exists with same track, reusing", offset);
            return readers[offset];
        }
        NET_LOG_WARN("Reader on %d already exists with different track, replacing", offset);
        readers[offset]->Stop();
        moq_session->UnsubscribeTrack(readers[offset]);
    }

    NET_LOG_WARN("Create reader %s:%s idx %d", channel_name.c_str(), codec.c_str(), offset);
    readers[offset].reset(
        new moq::TrackReader(moq::MakeFullTrackName(track_namespace, trackname), serial, codec));

    lock.unlock();
    return readers[offset];
}
catch (const std::exception& ex)
{
    ESP_LOGE("sub", "Exception in sub %s", ex.what());
    return nullptr;
}

std::shared_ptr<moq::TrackWriter> CreateWriteTrack(const std::string& channel_name,
                                                   const std::vector<std::string>& track_namespace,
                                                   const std::string& trackname,
                                                   const std::string& codec)
try
{
    uint32_t offset = 0;
    if (codec == "pcm")
    {
        if (channel_name == "ai_audio")
        {
            offset = (uint32_t)ui_net_link::Channel_Id::Ptt_Ai;
        }
        else
        {
            offset = (uint32_t)ui_net_link::Channel_Id::Ptt;
        }
    }
    else if (codec == "ascii")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat;
    }
    else
    {
        // do nothing with the writer if we don't know it.
        NET_LOG_ERROR("Unknown writer channel");
        return nullptr;
    }

    if (!moq_session)
    {
        NET_LOG_WARN("MoQ session not ready, cannot create writer");
        return nullptr;
    }

    auto desired_ftn = moq::MakeFullTrackName(track_namespace, trackname);

    std::unique_lock<std::mutex> lock = moq_session->GetWriterLock();
    if (writers[offset] != nullptr)
    {
        // If track already exists with same name, return it
        auto existing_ftn = writers[offset]->GetFullTrackName();
        if (existing_ftn.name_space == desired_ftn.name_space
            && existing_ftn.name == desired_ftn.name)
        {
            NET_LOG_INFO("Writer on %d already exists with same track, reusing", offset);
            return writers[offset];
        }
        NET_LOG_WARN("Writer on %d already exists with different track, replacing", offset);
        writers[offset]->Stop();
        moq_session->UnpublishTrack(writers[offset]);
    }

    NET_LOG_WARN("Create writer %s:%s idx %d", channel_name.c_str(), codec.c_str(), offset);
    writers[offset].reset(new moq::TrackWriter(moq::MakeFullTrackName(track_namespace, trackname),
                                               quicr::TrackMode::kDatagram, 2, 100));

    lock.unlock();
    return writers[offset];
}
catch (const std::exception& ex)
{
    ESP_LOGE("pub", "Exception in pub %s", ex.what());
    return nullptr;
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
    Diagnostics diagnostics;
    Storage storage;
    ConfigState config(storage);
    MgmtLinkHandler mgmt_link_handler(mgmt_layer, storage, config, diagnostics);

    Peripherals
        // Wifi wifi(storage);
        Runtime runtime{Wifi(storage)};

    SetPThreadDefault();
    PrintRAM();
    InitializeGPIO();
    IntitializeLEDs();
    InitializeUIReadyISR(GpioIsrRisingHandler);

    NET_LOG_INFO("Starting Net Main");

    CreateMgmtLinkPacketTask();

    CreateUILinkPacketTask();

    wifi.Begin();

#if defined(my_ssid) && defined(my_ssid_pwd)
    wifi.Connect(my_ssid, my_ssid_pwd);
#endif

    ui_layer.BeginEventTask(net_ui_serial_read_handle);

    // std::string& connect_uri = config.moq_server_url.Load();
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
    if (config.channel_ns.empty())
    {
        NET_LOG_WARN("Channel namespace not configured - device needs configuration");
    }
    if (config.ai_query_ns.empty() || config.ai_audio_response_ns.empty()
        || config.ai_cmd_response_ns.empty())
    {
        NET_LOG_WARN("AI namespaces not configured - device needs configuration");
    }

    // Use mac addr as id for my session
    uint64_t mac = 0;
    esp_efuse_mac_get_default((uint8_t*)&mac);
    mac = mac << 2;
    mac = mac >> 2;
    device_id = mac;

    NET_LOG_ERROR("mac addr %llu", mac);

    // setup moq transport
    quicr::ClientConfig moq_config;
    moq_config.endpoint_id = std::to_string(device_id);
    moq_config.connect_uri = config.moq_server_url.Load();
    moq_config.transport_config.debug = true;
    moq_config.transport_config.use_reset_wait_strategy = false;
    moq_config.transport_config.time_queue_max_duration = 5000;
    moq_config.transport_config.tls_cert_filename = "";
    moq_config.transport_config.tls_key_filename = "";
    moq_config.tick_service_sleep_delay_us = 30000;

    // Initialize reader/writer vectors
    readers.resize((uint32_t)ui_net_link::Channel_Id::Count);
    writers.resize((uint32_t)ui_net_link::Channel_Id::Count - 1);

    moq_session.reset(new moq::Session(moq_config, readers, writers));

    PrintRAM();

    NET_LOG_INFO("Components ready");

    // Set up initial tracks based on stored configuration
    UpdateAITracks();
    UpdateChannelTracks();

    moq::Session::Status prev_status = moq::Session::Status::kNotConnected;
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
                // Back to yellow
                gpio_set_level(NET_LED_R, 0);
                gpio_set_level(NET_LED_G, 0);
                gpio_set_level(NET_LED_B, 1);

                if (status == moq::Session::Status::kConnecting
                    || status == moq::Session::Status::kPendingServerSetup
                    || status == moq::Session::Status::kReady)
                {
                    RestartMoqSession(moq_session, moq_config, readers, writers);
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
                UpdateChannelTracks();
                UpdateAITracks();

                for (const auto& reader : readers)
                {
                    NET_LOG_INFO("Starting reader");
                    if (reader)
                    {
                        reader->Start();
                    }
                }

                for (const auto& writer : writers)
                {
                    if (writer)
                    {
                        writer->Start();
                    }
                }

                break;
            }
            case moq::Session::Status::kNotConnected:
                [[fallthrough]];
            case moq::Session::Status::kFailedToConnect:
            {
                RestartMoqSession(moq_session, moq_config, readers, writers);
                break;
            }
            case moq::Session::Status::kNotReady:
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
                NET_LOG_INFO("No handler for this moq state");
                break;
            }
            }
            prev_status = status;
        }

        if (moq_config.connect_uri != config.moq_server_url)
        {
            StopMoqSession(moq_session, readers, writers);

            NET_LOG_INFO("Load moq server uri");
            moq_config.connect_uri = config.moq_server_url;
            NET_LOG_INFO("Make a new session with uri %s", moq_config.connect_uri.c_str());
            moq_session.reset(new moq::Session(moq_config, readers, writers));
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

bool CreateUILinkPacketTask()
{
    constexpr size_t stack_size = 8092 * 2;
    net_ui_serial_read_stack =
        (StackType_t*)heap_caps_malloc(stack_size * sizeof(StackType_t), MALLOC_CAP_SPIRAM);
    if (net_ui_serial_read_stack == NULL)
    {
        NET_LOG_INFO("Failed to allocate stack for ui link packet handler");
        return false;
    }
    net_ui_serial_read_handle =
        xTaskCreateStatic(UILinkPacketTask, "ui link packet handler", stack_size, NULL, 10,
                          net_ui_serial_read_stack, &net_ui_serial_read_buffer);

    NET_LOG_INFO("Created ui link packet handler PSRAM left %ld",
                 heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return true;
}

void StopMoqSession(std::shared_ptr<moq::Session>& session,
                    std::vector<std::shared_ptr<moq::TrackReader>>& readers,
                    std::vector<std::shared_ptr<moq::TrackWriter>>& writers)
{
    moq_session->StopTracks();

    moq_session->Disconnect();

    for (const auto& reader : readers)
    {
        if (reader)
        {
            reader->Stop();
        }
    }

    for (const auto& writer : writers)
    {
        if (writer)
        {
            writer->Stop();
        }
    }

    gpio_set_level(NET_LED_B, 1);
}

void RestartMoqSession(std::shared_ptr<moq::Session>& session,
                       quicr::ClientConfig& config,
                       std::vector<std::shared_ptr<moq::TrackReader>>& readers,
                       std::vector<std::shared_ptr<moq::TrackWriter>>& writers)
{
    StopMoqSession(session, readers, writers);
    moq_session.reset(new moq::Session(config, readers, writers));
}

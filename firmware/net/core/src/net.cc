#include "net.hh"
#include "chunk.hh"
#include "config_builder.hh"
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
uint64_t device_id = 0;

// NOTE! This can be enabled during run on via serial
bool loopback = false;

std::vector<std::shared_ptr<moq::TrackReader>> readers;
std::vector<std::shared_ptr<moq::TrackWriter>> writers;

std::shared_ptr<moq::Session> moq_session;
SemaphoreHandle_t audio_req_smpr = xSemaphoreCreateBinary();

/** END EXTERNAL VARIABLES */
TaskHandle_t net_ui_serial_read_handle;
StaticTask_t net_ui_serial_read_buffer;
StackType_t* net_ui_serial_read_stack = nullptr;

uint8_t net_ui_uart_tx_buff[NET_UI_UART_TX_BUFF_SIZE] = {0};
uint8_t net_ui_uart_rx_buff[NET_UI_UART_RX_BUFF_SIZE] = {0};

uart_config_t net_ui_uart_config = {
    .baud_rate = 460800,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_2,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
    .flags = {},
};

Serial ui_layer(NET_UI_UART_PORT,
                NET_UI_UART_DEV,
                net_ui_serial_read_handle,
                ETS_UART1_INTR_SOURCE,
                net_ui_uart_config,
                NET_UI_UART_TX_PIN,
                NET_UI_UART_RX_PIN,
                UART_PIN_NO_CHANGE,
                UART_PIN_NO_CHANGE,
                *net_ui_uart_tx_buff,
                NET_UI_UART_TX_BUFF_SIZE,
                *net_ui_uart_rx_buff,
                NET_UI_UART_RX_BUFF_SIZE,
                NET_UI_UART_RING_RX_NUM,
                8192,
                8192,
                20,
                true,
                false);

TaskHandle_t net_mgmt_serial_read_handle;
StaticTask_t net_mgmt_serial_read_buffer;
StackType_t* net_mgmt_serial_read_stack = nullptr;

uint8_t net_mgmt_uart_tx_buff[NET_MGMT_UART_TX_BUFF_SIZE] = {0};
uint8_t net_mgmt_uart_rx_buff[NET_MGMT_UART_RX_BUFF_SIZE] = {0};

uart_config_t net_mgmt_uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .rx_flow_ctrl_thresh = 0,
    .source_clk = UART_SCLK_DEFAULT,
    .flags = {},
};

Serial mgmt_layer(UART_NUM_0,
                  UART0,
                  net_mgmt_serial_read_handle,
                  ETS_UART0_INTR_SOURCE,
                  net_mgmt_uart_config,
                  NET_MGMT_UART_TX_PIN,
                  NET_MGMT_UART_RX_PIN,
                  UART_PIN_NO_CHANGE,
                  UART_PIN_NO_CHANGE,
                  *net_mgmt_uart_tx_buff,
                  NET_MGMT_UART_TX_BUFF_SIZE,
                  *net_mgmt_uart_rx_buff,
                  NET_MGMT_UART_RX_BUFF_SIZE,
                  NET_MGMT_UART_RING_RX_NUM,
                  0,
                  256,
                  2,
                  true,
                  false);

Storage storage;
Wifi wifi(storage);
StoredValue<std::string> moq_server_url(storage, "moq", "server_url");
StoredValue<std::string> language(storage, "config", "language");
// Channel namespace (JSON-encoded array of strings)
StoredValue<std::string> channel_ns_json(storage, "config", "channel_ns");
// AI namespaces (JSON-encoded arrays of strings)
StoredValue<std::string> ai_query_ns_json(storage, "config", "ai_qry_ns");
StoredValue<std::string> ai_audio_response_ns_json(storage, "config", "ai_aud_ns");
StoredValue<std::string> ai_cmd_response_ns_json(storage, "config", "ai_cmd_ns");

// In-memory namespace caches (decoded from JSON)
std::vector<std::string> channel_ns;
std::vector<std::string> ai_query_ns;
std::vector<std::string> ai_audio_response_ns;
std::vector<std::string> ai_cmd_response_ns;

uint64_t curr_audio_isr_time = esp_timer_get_time();
uint64_t last_audio_isr_time = esp_timer_get_time();

bool logs_disabled = false;

spdlog::level::level_enum last_spd_log_level =
    static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL);

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

static void DisableLogging()
{
    esp_log_level_set("*", ESP_LOG_NONE);
    auto logger = spdlog::get("MTC");
    if (logger)
    {
        last_spd_log_level = logger->level();
        logger->set_level(spdlog::level::level_enum::off);
    }

    debug_printf_suspend();
    logs_disabled = true;
}

static void EnableLogging()
{
    // Renable
    esp_log_level_set("*", ESP_LOG_MAX);

    auto logger = spdlog::get("MTC");
    if (logger)
    {
        logger->set_level(last_spd_log_level);
    }

    debug_printf_resume();
    logs_disabled = false;
}

// Load namespaces from storage into memory
// Initialize empty entries to "[]" so storage always contains valid JSON
static void LoadNamespacesFromStorage()
{
    auto load_ns = [](StoredValue<std::string>& stored, std::vector<std::string>& ns) {
        std::string json_str = stored.Load();
        if (json_str.empty())
        {
            stored = "[]";
            ns.clear();
            return;
        }
        auto parsed = JsonToNamespace(json_str);
        ns = parsed.value_or(std::vector<std::string>{});
    };

    load_ns(channel_ns_json, channel_ns);
    load_ns(ai_query_ns_json, ai_query_ns);
    load_ns(ai_audio_response_ns_json, ai_audio_response_ns);
    load_ns(ai_cmd_response_ns_json, ai_cmd_response_ns);
}

// Update AI tracks when AI namespaces or language changes
// AI Query: publish to {ai_query_ns, language}
// AI Audio Response: subscribe to {ai_audio_response_ns, device_id}
// AI Command Response: subscribe to {ai_cmd_response_ns, device_id}
static void UpdateAITracks()
{
    const std::string lang = language.Load();
    const std::string device_id_str = std::to_string(device_id);

    // AI Query publication track
    if (!ai_query_ns.empty() && !lang.empty())
    {
        if (auto writer = CreateWriteTrack("ai_audio", ai_query_ns, lang, "pcm"))
        {
            writer->Start();
        }
    }

    // AI Audio Response subscription track
    if (!ai_audio_response_ns.empty())
    {
        if (auto reader = CreateReadTrack("self_ai_audio", ai_audio_response_ns, device_id_str,
                                          "pcm", ui_layer))
        {
            reader->Start();
        }
    }

    // AI Command Response subscription track
    if (!ai_cmd_response_ns.empty())
    {
        if (auto reader = CreateReadTrack("self_ai_text", ai_cmd_response_ns, device_id_str,
                                          "ai_cmd_response:json", ui_layer))
        {
            reader->Start();
        }
    }
}

// Update channel tracks when channel namespace or language changes
// Channel: both publish and subscribe to {channel_ns, language}
static void UpdateChannelTracks()
{
    const std::string lang = language.Load();

    if (channel_ns.empty() || lang.empty())
    {
        NET_LOG_WARN("Cannot update channel tracks: namespace or language empty");
        return;
    }

    // Channel publication track
    if (auto writer = CreateWriteTrack("channel", channel_ns, lang, "pcm"))
    {
        writer->Start();
    }

    // Channel subscription track
    if (auto reader = CreateReadTrack("channel", channel_ns, lang, "pcm", ui_layer))
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
            if ((ui_net_link::Packet_Type)packet->type != ui_net_link::Packet_Type::Message)
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

static void MgmtLinkPacketTask(void* args)
{
    // TODO need to use TLV not slip, serial should take a param for that oops
    NET_LOG_INFO("Start mgmt link packet task");
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (auto packet = mgmt_layer.Read())
        {
            switch (packet->type)
            {
            case Configuration::Version:
            {
                NET_LOG_WARN("VERSION NOT IMPLEMENTED");
                break;
            }
            case Configuration::Clear_Storage:
            {
                if (ESP_OK != storage.Clear())
                {
                    mgmt_layer.ReplyNack();
                    break;
                }

                mgmt_layer.ReplyAck();
                break;
            }
            case Configuration::Set_Ssid:
            {
                // Parse ssid from packet, first four bytes of payload will be
                // the ssid name length and so on
                uint32_t ssid_name_len = 0;
                uint32_t ssid_password_len = 0;

                std::span<uint8_t> payload{packet->payload};

                std::memcpy(&ssid_name_len, payload.data(), sizeof(ssid_name_len));
                payload = payload.subspan(sizeof(ssid_name_len));

                std::string ssid_name(reinterpret_cast<char*>(payload.data()), ssid_name_len);
                payload = payload.subspan(ssid_name_len);

                std::memcpy(&ssid_password_len, payload.data(), sizeof(ssid_password_len));
                payload = payload.subspan(sizeof(ssid_password_len));

                std::string ssid_password(reinterpret_cast<char*>(payload.data()),
                                          ssid_password_len);
                payload = payload.subspan(ssid_password_len);

                NET_LOG_INFO("Got ssid name len %lu %s", ssid_name_len, ssid_name.c_str());
                NET_LOG_INFO("Got ssid password len %lu %s", ssid_password_len,
                             ssid_password.c_str());

                wifi.Connect(ssid_name, ssid_password);

                mgmt_layer.ReplyAck();
                break;
            }
            case Configuration::Get_Ssid_Names:
            {
                std::string str = "";
                const std::vector<Wifi::ap_cred_t>& creds = wifi.GetStoredCreds();
                for (size_t i = 0; i < creds.size(); ++i)
                {
                    if (i > 0)
                    {
                        str.append(",");
                    }
                    str.append(creds[i].name, creds[i].name_len);
                }

                mgmt_layer.ReplyData(str);
                break;
            }
            case Configuration::Get_Ssid_Passwords:
            {
                std::string str = "";
                const std::vector<Wifi::ap_cred_t>& creds = wifi.GetStoredCreds();
                for (size_t i = 0; i < creds.size(); ++i)
                {
                    if (i > 0)
                    {
                        str.append(",");
                    }
                    str.append(creds[i].pwd, creds[i].pwd_len);
                }
                mgmt_layer.ReplyData(str);
                break;
            }
            case Configuration::Clear_Ssids:
            {
                wifi.ClearSavedSSIDs();
                mgmt_layer.ReplyAck();
                break;
            }
            case Configuration::Set_Moq_Url:
            {
                std::string moq_url((char*)packet->payload.data(), packet->length);
                NET_LOG_INFO("Got moq url %d - %s", moq_url.length(), moq_url.c_str());

                if (moq_url.length() == 0)
                {
                    // MOQ url will be cleared.
                    NET_LOG_INFO("Cleaning moq url because length is zero");
                    moq_server_url.Clear();
                    mgmt_layer.ReplyAck();
                    break;
                }
                moq_server_url = moq_url;
                mgmt_layer.ReplyAck();
                break;
            }
            case Configuration::Get_Moq_Url:
            {
                std::string moq_url = moq_server_url.Load();
                mgmt_layer.ReplyData(moq_url);
                break;
            }
            case Configuration::Get_Loopback:
            {
                // Return loopback mode: 0=off, 1=raw (local bypass)
                uint8_t mode = loopback ? 1 : 0;
                mgmt_layer.ReplyData(std::string(1, static_cast<char>(mode)));
                break;
            }
            case Configuration::Set_Loopback:
            {
                if (packet->length < 1)
                {
                    mgmt_layer.ReplyNack();
                    break;
                }
                uint8_t mode = packet->payload[0];
                loopback = (mode != 0);
                NET_LOG_INFO("Loopback set to: %s", loopback ? "on" : "off");
                mgmt_layer.ReplyAck();
                break;
            }
            case Configuration::Get_Logs_Enabled:
            {
                // Return logs state: 0=disabled, 1=enabled
                uint8_t enabled = logs_disabled ? 0 : 1;
                mgmt_layer.ReplyData(std::string(1, static_cast<char>(enabled)));
                break;
            }
            case Configuration::Set_Logs_Enabled:
            {
                if (packet->length < 1)
                {
                    mgmt_layer.ReplyNack();
                    break;
                }
                uint8_t enabled = packet->payload[0];
                if (enabled)
                {
                    mgmt_layer.ReplyAck();
                    EnableLogging();
                }
                else
                {
                    mgmt_layer.ReplyAck();
                    DisableLogging();
                }
                break;
            }
            case Configuration::Set_Language:
            {
                std::string new_lang((char*)packet->payload.data(), packet->length);
                if (!IsValidLanguage(new_lang))
                {
                    NET_LOG_ERROR("Invalid language: %s", new_lang.c_str());
                    mgmt_layer.ReplyNack();
                    break;
                }
                language = new_lang;
                NET_LOG_INFO("Language set to: %s", new_lang.c_str());

                // Send ACK before track updates to avoid interleaving with log output
                mgmt_layer.ReplyAck();

                // Update tracks that depend on language
                UpdateAITracks();
                UpdateChannelTracks();
                break;
            }
            case Configuration::Get_Language:
            {
                std::string lang = language.Load();
                mgmt_layer.ReplyData(lang);
                break;
            }
            case Configuration::Set_Channel:
            {
                std::string json_str((char*)packet->payload.data(), packet->length);
                auto parsed = JsonToNamespace(json_str);
                if (!parsed.has_value())
                {
                    NET_LOG_ERROR("Failed to parse channel namespace JSON");
                    mgmt_layer.ReplyNack();
                    break;
                }

                channel_ns = parsed.value();
                channel_ns_json = json_str;
                NET_LOG_INFO("Channel namespace set (%zu parts)", channel_ns.size());

                // Send ACK before track updates to avoid interleaving with log output
                mgmt_layer.ReplyAck();
                UpdateChannelTracks();
                break;
            }
            case Configuration::Get_Channel:
            {
                mgmt_layer.ReplyData(channel_ns_json.Load());
                break;
            }
            case Configuration::Set_AI:
            {
                std::string json_str((char*)packet->payload.data(), packet->length);
                try
                {
                    json ai_config = json::parse(json_str);
                    if (!ai_config.is_object() || !ai_config.contains("query")
                        || !ai_config.contains("audio") || !ai_config.contains("cmd"))
                    {
                        NET_LOG_ERROR("AI config must be object with query, audio, cmd fields");
                        mgmt_layer.ReplyNack();
                        break;
                    }

                    auto query_parsed = JsonToNamespace(ai_config["query"].dump());
                    auto audio_parsed = JsonToNamespace(ai_config["audio"].dump());
                    auto cmd_parsed = JsonToNamespace(ai_config["cmd"].dump());

                    if (!query_parsed || !audio_parsed || !cmd_parsed)
                    {
                        NET_LOG_ERROR("Failed to parse AI namespace arrays");
                        mgmt_layer.ReplyNack();
                        break;
                    }

                    ai_query_ns = query_parsed.value();
                    ai_audio_response_ns = audio_parsed.value();
                    ai_cmd_response_ns = cmd_parsed.value();

                    ai_query_ns_json = ai_config["query"].dump();
                    ai_audio_response_ns_json = ai_config["audio"].dump();
                    ai_cmd_response_ns_json = ai_config["cmd"].dump();

                    NET_LOG_INFO("AI namespaces set (query=%zu, audio=%zu, cmd=%zu parts)",
                                 ai_query_ns.size(), ai_audio_response_ns.size(),
                                 ai_cmd_response_ns.size());

                    // Send ACK before track updates to avoid interleaving with log output
                    mgmt_layer.ReplyAck();

                    UpdateAITracks();
                }
                catch (const std::exception& ex)
                {
                    NET_LOG_ERROR("Failed to parse AI config JSON: %s", ex.what());
                    mgmt_layer.ReplyNack();
                }
                break;
            }
            case Configuration::Get_AI:
            {
                json response;
                response["query"] = ai_query_ns;
                response["audio"] = ai_audio_response_ns;
                response["cmd"] = ai_cmd_response_ns;
                mgmt_layer.ReplyData(response.dump());
                break;
            }
            case Configuration::Burn_Disable_USB_JTag_Efuse:
            {
                const bool res = BurnDisableUSBJTagEFuse();
                if (res)
                {
                    mgmt_layer.ReplyAck();
                }
                else
                {
                    mgmt_layer.ReplyNack();
                }
                break;
            }
            default:
            {
                NET_LOG_ERROR("Unknown packet type from mgmt");
                break;
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
    SetPThreadDefault();
    PrintRAM();
    InitializeGPIO();
    IntitializeLEDs();
    InitializeUIReadyISR(GpioIsrRisingHandler);

    NET_LOG_INFO("Starting Net Main");

    CreateMgmtLinkPacketTask();
    mgmt_layer.BeginEventTask();

    CreateUILinkPacketTask();

    wifi.Begin();

#if defined(my_ssid) && defined(my_ssid_pwd)
    wifi.Connect(my_ssid, my_ssid_pwd);
#endif

    ui_layer.BeginEventTask();

    // std::string& connect_uri = moq_server_url.Load();
    if (moq_server_url->empty())
    {
        // No moq url found, using default
        NET_LOG_WARN(
            "No moq server url found, using default moq://relay.us-west-2.quicr.ctgpoc.com:33435");
        moq_server_url = "moq://relay.us-west-2.quicr.ctgpoc.com:33435";
    }

    NET_LOG_WARN("Using moq server url of %s len %u", moq_server_url->c_str(),
                 moq_server_url->length());

    // Set default language if not configured
    if (language->empty())
    {
        language = "en-US";
    }

    // Load namespaces from storage
    LoadNamespacesFromStorage();

    // Log warnings if namespaces are not configured (no defaults - must be configured via
    // fl-identity)
    if (channel_ns.empty())
    {
        NET_LOG_WARN("Channel namespace not configured - device needs configuration");
    }
    if (ai_query_ns.empty() || ai_audio_response_ns.empty() || ai_cmd_response_ns.empty())
    {
        NET_LOG_WARN("AI namespaces not configured - device needs configuration");
    }

    // setup moq transport
    quicr::ClientConfig config;
    config.endpoint_id = "hactar-ev12-snk";
    config.connect_uri = moq_server_url.Load();
    config.transport_config.debug = true;
    config.transport_config.use_reset_wait_strategy = false;
    config.transport_config.time_queue_max_duration = 5000;
    config.transport_config.tls_cert_filename = "";
    config.transport_config.tls_key_filename = "";
    config.tick_service_sleep_delay_us = 30000;

    // Use mac addr as id for my session
    uint64_t mac = 0;
    esp_efuse_mac_get_default((uint8_t*)&mac);
    mac = mac << 2;
    mac = mac >> 2;
    device_id = mac;

    NET_LOG_ERROR("mac addr %llu", mac);

    // Initialize reader/writer vectors
    readers.resize((uint32_t)ui_net_link::Channel_Id::Count);
    writers.resize((uint32_t)ui_net_link::Channel_Id::Count - 1);

    moq_session.reset(new moq::Session(config, readers, writers));

    PrintRAM();

    NET_LOG_INFO("Components ready");

    // Set up initial tracks based on stored configuration
    UpdateAITracks();
    UpdateChannelTracks();

    int next = 0;
    int64_t heartbeat = 0;
    bool ready_to_connect_moq = false;
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
                    RestartMoqSession(moq_session, config, readers, writers);
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
                RestartMoqSession(moq_session, config, readers, writers);
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

        if (config.connect_uri != moq_server_url)
        {
            StopMoqSession(moq_session, readers, writers);

            NET_LOG_INFO("Load moq server uri");
            config.connect_uri = moq_server_url;
            NET_LOG_INFO("Make a new session with uri %s", config.connect_uri.c_str());
            moq_session.reset(new moq::Session(config, readers, writers));
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

bool CreateMgmtLinkPacketTask()
{
    NET_LOG_INFO("Creating mgmt link packet task");

    constexpr size_t stack_size = 8192; // Increased for JSON parsing
    net_mgmt_serial_read_stack =
        (StackType_t*)heap_caps_malloc(stack_size * sizeof(StackType_t), MALLOC_CAP_INTERNAL);
    if (net_mgmt_serial_read_stack == NULL)
    {
        NET_LOG_INFO("Failed to allocate stack for mgmt link packet handler");
        return false;
    }
    net_mgmt_serial_read_handle =
        xTaskCreateStatic(MgmtLinkPacketTask, "mgmt link packet handler", stack_size, NULL, 10,
                          net_mgmt_serial_read_stack, &net_mgmt_serial_read_buffer);

    NET_LOG_INFO("Created mgmt link packet handler Internal RAM left %ld",
                 heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
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

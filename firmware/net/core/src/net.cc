#include "net.hh"
#include "chunk.hh"
#include "default_json.hh"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_pthread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "logger.hh"
#include "macros.hh"
#include "net_mgmt_link.h"
#include "nvs_flash.h"
#include "peripherals.hh"
#include "sdkconfig.h"
#include "serial.hh"
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

/** EXTERNAL VARIABLES */
// External variables defined in net.hh
uint64_t device_id = 0;

// TODO make this a config that can be changed using mgmt
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
    .source_clk = UART_SCLK_DEFAULT // UART_SCLK_DEFAULT
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
                2048,
                4096,
                20,
                true,
                true);

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
                break;
            }
            case ui_net_link::Packet_Type::TalkStart:
                break;
            case ui_net_link::Packet_Type::TalkStop:
                talk_stopped = true;
                break;
            case ui_net_link::Packet_Type::AiResponse:
            {
                NET_LOG_INFO("got ai response from ui");
                // Check if json for sure
                uint8_t channel_id = packet->payload[0];
                auto* response = static_cast<ui_net_link::AIResponseChunk*>(
                    static_cast<void*>(packet->payload + 1));

                if (!json::accept(response->chunk_data))
                {
                    NET_LOG_ERROR("Received invalid json");
                    break;
                }

                json change_channel = json::parse(response->chunk_data);
                NET_LOG_INFO("start track write for new channel");
                if (auto writer = CreateWriteTrack(change_channel))
                {
                    writer->Start();
                }

                NET_LOG_INFO("start track read for new channel");
                if (auto reader = CreateReadTrack(change_channel, ui_layer))
                {
                    reader->Start();
                }
                break;
            }
            case ui_net_link::Packet_Type::Message:
            {
                // Channel id is always zero, so, gotta fix that.
                uint8_t channel_id = packet->payload[0];
                uint32_t ext_bytes = 1;
                uint32_t length = packet->length;

                // Remove the bytes already read from the payload length
                length -= ext_bytes;

                if (channel_id < (uint8_t)ui_net_link::Channel_Id::Count - 1)
                {
                    if (auto& writer = writers[channel_id])
                    {
                        writers[channel_id]->PushObject(packet->payload + 1, length,
                                                        curr_audio_isr_time);
                    }
                }

                // TODO use notifies, currently it doesn't notify fast enough?
                // xTaskNotifyGive(rtos_pub_handle);

                break;
            }
            case ui_net_link::Packet_Type::WifiConnect:
            {
                NET_LOG_INFO("Got a wifi connect packet");
                // Get the SSID and password from the packet.

                std::string ssid;
                std::string pwd;

                ui_net_link::Deserialize(*packet, ssid, pwd);

                wifi.Connect(ssid, pwd);

                break;
            }
            default:
                NET_LOG_ERROR("Got a packet without a handler %d", (int)packet->type);
                break;
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
                NET_LOG_WARN("CLEAR STORAGE NOT IMPLEMENTED");
                break;
            }
            case Configuration::Set_Ssid:
            {
                // Parse ssid from packet, first four bytes of payload will be
                // the ssid name length and so on
                uint32_t ssid_name_len = 0;
                uint32_t ssid_password_len = 0;

                uint32_t offset = 0;

                packet->Get(&ssid_name_len, sizeof(ssid_name_len), offset);
                offset += sizeof(ssid_name_len);

                std::string ssid_name((char*)packet->payload + offset, ssid_name_len);
                offset += ssid_name_len;

                packet->Get(&ssid_password_len, sizeof(ssid_password_len), offset);
                offset += sizeof(ssid_password_len);

                std::string ssid_password((char*)packet->payload + offset, ssid_password_len);
                offset += ssid_password_len;

                NET_LOG_INFO("Got ssid name len %lu %s", ssid_name_len, ssid_name.c_str());
                NET_LOG_INFO("Got ssid password len %lu %s", ssid_password_len,
                             ssid_password.c_str());

                wifi.Connect(ssid_name, ssid_password);
                break;
            }
            case Configuration::Get_Ssid_Names:
            {
                std::string str = "Saved SSIDS Names:";
                const std::vector<Wifi::ap_cred_t>& creds = wifi.GetStoredCreds();
                for (size_t i = 0; i < creds.size(); ++i)
                {
                    str.append("\n");
                    str.append(creds[i].name, creds[i].name_len);
                }

                mgmt_layer.Write((uint8_t*)str.data(), str.length());
                break;
            }
            case Configuration::Get_Ssid_Passwords:
            {
                std::string str = "Saved SSID Passwords:";
                const std::vector<Wifi::ap_cred_t>& creds = wifi.GetStoredCreds();
                for (size_t i = 0; i < creds.size(); ++i)
                {
                    str.append("\n");
                    str.append(creds[i].pwd, creds[i].pwd_len);
                }
                mgmt_layer.Write((uint8_t*)str.data(), str.length());
                break;
            }
            case Configuration::Clear_Ssids:
            {
                wifi.ClearSavedSSIDs();
                break;
            }
            case Configuration::Set_Moq_Url:
            {
                std::string moq_url((char*)packet->payload, packet->length);
                NET_LOG_INFO("Got moq url %d - %s", moq_url.length(), moq_url.c_str());

                if (moq_url.length() == 0)
                {
                    // MOQ url will be cleared.
                    NET_LOG_INFO("Cleaning moq url because length is zero");
                    moq_server_url.Clear();
                    break;
                }
                moq_server_url = moq_url;
                break;
            }
            case Configuration::Get_Moq_Url:
            {
                std::string moq_url = "Saved moq url: ";
                moq_url += moq_server_url.Load();
                mgmt_layer.Write((uint8_t*)moq_url.data(), moq_url.length());
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

std::shared_ptr<moq::TrackReader> CreateReadTrack(const json& subscription, Serial& serial)
try
{
    std::vector<std::string> track_namespace =
        subscription.at("tracknamespace").get<std::vector<std::string>>();
    std::string trackname = subscription.at("trackname").get<std::string>();
    if (trackname.empty())
    {
        trackname = std::to_string(device_id);
    }

    std::string codec = subscription.at("codec").get<std::string>();
    std::string channel_name = subscription.at("channel_name").get<std::string>();

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

    std::unique_lock<std::mutex> lock = moq_session->GetReaderLock();
    if (readers[offset] != nullptr)
    {
        NET_LOG_WARN("Reader on %d already exists", offset);
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

std::shared_ptr<moq::TrackWriter> CreateWriteTrack(const json& publication)
try
{
    std::vector<std::string> track_namespace =
        publication.at("tracknamespace").get<std::vector<std::string>>();
    std::string trackname = publication.at("trackname").get<std::string>();

    std::string codec = publication.at("codec").get<std::string>();
    std::string channel_name = publication.at("channel_name").get<std::string>();

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

    std::unique_lock<std::mutex> lock = moq_session->GetWriterLock();
    if (writers[offset] != nullptr)
    {
        NET_LOG_WARN("Writer on %d already exists", offset);
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

    CreateMgmtLinkPacketTask();
    mgmt_layer.BeginEventTask();

    CreateUILinkPacketTask();

    NET_LOG_INFO("Starting Net Main");

    wifi.Begin();

    wifi.Connect("quicr.io", "noPassword");
    wifi.Connect("m10x-interference", "goodlife");

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

    moq_session.reset(new moq::Session(config, readers, writers));

    PrintRAM();

    NET_LOG_INFO("Components ready");

    json subscriptions = default_channel_json.at("subscriptions");
    readers.resize((uint32_t)ui_net_link::Channel_Id::Count);
    NET_LOG_ERROR("Readers size %d", readers.size());
    for (int i = 0; i < subscriptions.size(); ++i)
    {
        CreateReadTrack(subscriptions[i], ui_layer);
    }
    NET_LOG_ERROR("Readers size %d", readers.size());

    json publications = default_channel_json.at("publications");
    // Kinda odd, but we only have 3 writers.
    writers.resize((uint32_t)ui_net_link::Channel_Id::Count - 1);
    NET_LOG_ERROR("Writers size %d", writers.size());
    for (int i = 0; i < publications.size(); ++i)
    {
        CreateWriteTrack(publications[i]);
    }

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

            link_packet_t packet;
            packet.type = (uint8_t)ui_net_link::Packet_Type::WifiStatus;
            packet.payload[0] = (uint8_t)wifi_state;
            ui_layer.Write(packet);

            switch (wifi_state)
            {
            case Wifi::State::Disconnected:
            {
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

                // TODO
                // Tell ui chip we are ready
                gpio_set_level(NET_LED_B, 0);
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
                gpio_set_level(NET_LED_B, 1);

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

    constexpr size_t stack_size = 4096;
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
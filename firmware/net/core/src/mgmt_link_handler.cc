#include "mgmt_link_handler.hh"
#include "freertos/FreeRTOS.h"
#include "logger.hh"

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

static void MgmtLinkPacketTask(void* args)
{
    NET_LOG_INFO("Start mgmt link packet task");
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (auto packet = mgmt_layer.Read())
        {
            switch (static_cast<CtlToNet>(packet->type))
            {
            case CtlToNet::Ping:
            {
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Pong),
                                 std::span<const uint8_t>(packet->payload.data(), packet->length));
                break;
            }
            case CtlToNet::CircularPing:
            {
                // Forward to UI for circular path: MGMT -> NET -> UI -> MGMT
                ui_layer.Reply(static_cast<uint16_t>(ui_net_link::NetToUi::CircularPing),
                               std::span<const uint8_t>(packet->payload.data(), packet->length));
                break;
            }
            case CtlToNet::ClearStorage:
            {
                if (ESP_OK != storage.Clear())
                {
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Storage clear failed");
                    break;
                }

                // Clear in-memory caches (NVS already wiped by storage.Clear())
                moq_server_url.stored.clear();
                language.stored.clear();
                channel_ns_json.stored.clear();
                ai_query_ns_json.stored.clear();
                ai_audio_response_ns_json.stored.clear();
                ai_cmd_response_ns_json.stored.clear();

                // Clear in-memory namespace vectors
                channel_ns.clear();
                ai_query_ns.clear();
                ai_audio_response_ns.clear();
                ai_cmd_response_ns.clear();

                // Clear WiFi credentials
                wifi.ClearSavedSSIDs();

                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                break;
            }
            case CtlToNet::AddWifiSsid:
            {
                try
                {
                    std::string json_str((char*)packet->payload.data(), packet->length);
                    json wifi_json = json::parse(json_str);

                    if (!wifi_json.contains("ssid") || !wifi_json.contains("password"))
                    {
                        NET_LOG_ERROR("Add_Wifi: missing ssid or password field");
                        mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                              "Missing ssid or password field");
                        break;
                    }

                    std::string ssid = wifi_json["ssid"].get<std::string>();
                    std::string password = wifi_json["password"].get<std::string>();

                    NET_LOG_INFO("Add_Wifi: ssid=%s", ssid.c_str());
                    wifi.Connect(ssid, password);
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                }
                catch (const std::exception& ex)
                {
                    NET_LOG_ERROR("Add_Wifi: JSON parse error: %s", ex.what());
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error), "Invalid JSON");
                }
                break;
            }
            case CtlToNet::GetWifiSsids:
            {
                json wifi_array = json::array();
                const std::vector<Wifi::ap_cred_t>& creds = wifi.GetStoredCreds();
                for (const auto& cred : creds)
                {
                    json entry;
                    entry["ssid"] = std::string(cred.name, cred.name_len);
                    entry["password"] = std::string(cred.pwd, cred.pwd_len);
                    wifi_array.push_back(entry);
                }
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::WifiSsids), wifi_array.dump());
                break;
            }
            case CtlToNet::ClearWifiSsids:
            {
                wifi.ClearSavedSSIDs();
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                break;
            }
            case CtlToNet::SetRelayUrl:
            {
                std::string moq_url((char*)packet->payload.data(), packet->length);
                NET_LOG_INFO("Got moq url %d - %s", moq_url.length(), moq_url.c_str());

                if (moq_url.length() == 0)
                {
                    NET_LOG_INFO("Cleaning moq url because length is zero");
                    moq_server_url.Clear();
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                    break;
                }
                moq_server_url = moq_url;
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                break;
            }
            case CtlToNet::GetRelayUrl:
            {
                std::string moq_url = moq_server_url.Load();
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::RelayUrl), moq_url);
                break;
            }
            case CtlToNet::GetLoopback:
            {
                uint8_t mode =
                    static_cast<uint8_t>(loopback ? NetLoopbackMode::Moq : NetLoopbackMode::Off);
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Loopback),
                                 std::span<const uint8_t>(&mode, 1));
                break;
            }
            case CtlToNet::SetLoopback:
            {
                if (packet->length < 1)
                {
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Missing loopback mode parameter");
                    break;
                }
                auto mode = static_cast<NetLoopbackMode>(packet->payload[0]);
                switch (mode)
                {
                case NetLoopbackMode::Off:
                    loopback = false;
                    NET_LOG_INFO("Loopback set to: off");
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                    break;
                case NetLoopbackMode::Raw:
                    NET_LOG_WARN("Loopback mode 'raw' not supported");
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Raw loopback not supported");
                    break;
                case NetLoopbackMode::Moq:
                    loopback = true;
                    NET_LOG_INFO("Loopback set to: moq");
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                    break;
                default:
                    NET_LOG_WARN("Unknown loopback mode: %d", static_cast<int>(mode));
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Unknown loopback mode");
                    break;
                }
                break;
            }
            case CtlToNet::GetLogsEnabled:
            {
                uint8_t enabled = logs_disabled ? 0 : 1;
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::LogsEnabled),
                                 std::span<const uint8_t>(&enabled, 1));
                break;
            }
            case CtlToNet::SetLogsEnabled:
            {
                if (packet->length < 1)
                {
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Missing logs enabled parameter");
                    break;
                }
                uint8_t enabled = packet->payload[0];
                if (enabled)
                {
                    EnableLogging();
                }
                else
                {
                    DisableLogging();
                }
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                break;
            }
            case CtlToNet::SetLanguage:
            {
                std::string new_lang((char*)packet->payload.data(), packet->length);
                if (!IsValidLanguage(new_lang))
                {
                    NET_LOG_ERROR("Invalid language: %s", new_lang.c_str());
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Invalid language");
                    break;
                }
                language = new_lang;
                NET_LOG_INFO("Language set to: %s", new_lang.c_str());
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                UpdateAITracks();
                UpdateChannelTracks();
                break;
            }
            case CtlToNet::GetLanguage:
            {
                std::string lang = language.Load();
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Language), lang);
                break;
            }
            case CtlToNet::SetChannel:
            {
                std::string json_str((char*)packet->payload.data(), packet->length);
                auto parsed = JsonToNamespace(json_str);
                if (!parsed.has_value())
                {
                    NET_LOG_ERROR("Failed to parse channel namespace JSON");
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Invalid channel namespace JSON");
                    break;
                }

                channel_ns = parsed.value();
                channel_ns_json = json_str;
                NET_LOG_INFO("Channel namespace set (%zu parts)", channel_ns.size());
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack), std::span<const uint8_t>{});
                UpdateChannelTracks();
                break;
            }
            case CtlToNet::GetChannel:
            {
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Channel), channel_ns_json.Load());
                break;
            }
            case CtlToNet::SetAi:
            {
                std::string json_str((char*)packet->payload.data(), packet->length);
                try
                {
                    json ai_config = json::parse(json_str);
                    if (!ai_config.is_object() || !ai_config.contains("query")
                        || !ai_config.contains("audio") || !ai_config.contains("cmd"))
                    {
                        NET_LOG_ERROR("AI config must be object with query, audio, cmd fields");
                        mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                              "AI config requires query, audio, cmd fields");
                        break;
                    }

                    auto query_parsed = JsonToNamespace(ai_config["query"].dump());
                    auto audio_parsed = JsonToNamespace(ai_config["audio"].dump());
                    auto cmd_parsed = JsonToNamespace(ai_config["cmd"].dump());

                    if (!query_parsed || !audio_parsed || !cmd_parsed)
                    {
                        NET_LOG_ERROR("Failed to parse AI namespace arrays");
                        mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                              "Invalid AI namespace arrays");
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
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                    UpdateAITracks();
                }
                catch (const std::exception& ex)
                {
                    NET_LOG_ERROR("Failed to parse AI config JSON: %s", ex.what());
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "Invalid AI config JSON");
                }
                break;
            }
            case CtlToNet::GetAi:
            {
                json response;
                response["query"] = ai_query_ns;
                response["audio"] = ai_audio_response_ns;
                response["cmd"] = ai_cmd_response_ns;
                mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ai), response.dump());
                break;
            }
            case CtlToNet::BurnJtagEfuse:
            {
                const bool res = BurnDisableUSBJTagEFuse();
                if (res)
                {
                    mgmt_layer.Reply(static_cast<uint16_t>(NetToCtl::Ack),
                                     std::span<const uint8_t>{});
                }
                else
                {
                    mgmt_layer.ReplyError(static_cast<uint16_t>(NetToCtl::Error),
                                          "eFuse burn failed");
                }
                break;
            }
            default:
            {
                NET_LOG_ERROR("Unknown packet type 0x%04x from mgmt", packet->type);
                break;
            }
            }
        }
    }
}

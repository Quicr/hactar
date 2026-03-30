#include "link_packet_handler.hh"
#include "audio_chip.hh"
#include "audio_codec.hh"
#include "keyboard_display.hh"
#include "logger.hh"
#include "stack_debug.hh"
#include "ui_mgmt_link.h"
#include "ui_net_link.hh"
#include <cstdio>
#include <span>

static void HandleMedia(link_packet_t* packet, AudioChip& audio)
{
    ui_net_link::AudioObject play_frame;
    ui_net_link::Deserialize(*packet, play_frame);
    AudioCodec::ALawExpand(play_frame.data, constants::Audio_Phonic_Sz, audio.TxBuffer(),
                           constants::Audio_Buffer_Sz, constants::Stereo, true);
}

static void HandleAiResponse(link_packet_t* packet, AudioChip& audio)
{
    auto* response =
        static_cast<ui_net_link::AIResponseChunk*>(static_cast<void*>(packet->payload.data() + 1));

    switch (response->content_type)
    {
    case ui_net_link::ContentType::Audio:
    {
        const uint16_t len = constants::Audio_Phonic_Sz < response->chunk_length
                               ? constants::Audio_Phonic_Sz
                               : response->chunk_length;

        AudioCodec::ALawExpand(response->chunk_data, len, audio.TxBuffer(),
                               constants::Audio_Buffer_Sz, constants::Stereo, true);
        break;
    }
    case ui_net_link::ContentType::Json:
    {
        response->chunk_data[response->chunk_length] = 0;
        UI_LOG_INFO("[AI] %s", response->chunk_data);
        break;
    }
    }
}

void HandleNetLinkPackets(Serial& net_serial, Serial& mgmt_serial, Protector& protector, AudioChip& audio, Screen& screen)
{
    while (true)
    {
        link_packet_t* packet = net_serial.Read();
        if (!packet)
        {
            return;
        }

        if (packet->type == static_cast<uint16_t>(ui_net_link::NetToUi::CircularPing))
        {
            // Forward to MGMT for circular path: MGMT -> NET -> UI -> MGMT
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::CircularPing),
                              std::span<const uint8_t>(packet->payload.data(), packet->length));
            continue;
        }

        if (packet->type != static_cast<uint16_t>(ui_net_link::NetToUi::AudioFrame))
        {
            UI_LOG_ERROR("Unhandled packet type %d", (int)packet->type);
            continue;
        }

        if (!protector.TryUnprotect(packet))
        {
            UI_LOG_ERROR("Failed to decrypt ptt object");
            continue;
        }

        const auto message_type = static_cast<ui_net_link::MessageType>(packet->payload[1]);

        switch (message_type)
        {
        case ui_net_link::MessageType::Media:
        {
            HandleMedia(packet, audio);
            break;
        }
        case ui_net_link::MessageType::AIRequest:
        {
            break;
        }
        case ui_net_link::MessageType::AIResponse:
        {
            HandleAiResponse(packet, audio);
            break;
        }
        case ui_net_link::MessageType::Chat:
        {
            HandleChatMessages(screen, packet);
            break;
        }
        default:
        {
            UI_LOG_INFO("Got an unknown message type %d", (int)message_type);
            break;
        }
        }
    }
}

void HandleMgmtLinkPackets(Serial& mgmt_serial, Serial& net_serial, ConfigStorage& storage)
{
    while (true)
    {
        link_packet_t* packet = mgmt_serial.Read();
        if (!packet)
        {
            break;
        }

        switch (static_cast<CtlToUi>(packet->type))
        {
        case CtlToUi::Ping:
        {
            if (packet->length > 0)
            {
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Pong),
                                  std::span<const uint8_t>(packet->payload.data(), packet->length));
            }
            else
            {
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Pong), std::span<const uint8_t>{});
            }
            break;
        }
        case CtlToUi::CircularPing:
        {
            // Forward to NET for circular path: MGMT -> UI -> NET -> MGMT
            net_serial.Reply(static_cast<uint16_t>(ui_net_link::UiToNet::CircularPing),
                             std::span<const uint8_t>(packet->payload.data(), packet->length));
            break;
        }
        case CtlToUi::GetVersion:
        {
            uint32_t version = storage.GetVersion();
            uint8_t buf[4];
            buf[0] = static_cast<uint8_t>((version >> 24) & 0xFF);
            buf[1] = static_cast<uint8_t>((version >> 16) & 0xFF);
            buf[2] = static_cast<uint8_t>((version >> 8) & 0xFF);
            buf[3] = static_cast<uint8_t>(version & 0xFF);
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Version), std::span<const uint8_t>(buf, 4));
            break;
        }
        case CtlToUi::SetVersion:
        {
            if (packet->length != 4)
            {
                UI_LOG_ERROR("ERR. Version must be 4 bytes");
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Version must be 4 bytes");
                break;
            }
            uint32_t version = (static_cast<uint32_t>(packet->payload[0]) << 24)
                             | (static_cast<uint32_t>(packet->payload[1]) << 16)
                             | (static_cast<uint32_t>(packet->payload[2]) << 8)
                             | static_cast<uint32_t>(packet->payload[3]);
            if (storage.SetVersion(version))
            {
                UI_LOG_INFO("OK! Version set to 0x%08lx", version);
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            }
            else
            {
                UI_LOG_ERROR("ERR. Failed to set version");
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Failed to set version");
            }
            break;
        }
        case CtlToUi::ClearStorage:
        {
            UI_LOG_INFO("OK! Clearing configurations");
            storage.Clear();
            UI_LOG_INFO("OK! Cleared all configurations");
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            break;
        }
        case CtlToUi::SetSframeKey:
        {
            if (packet->length != 16)
            {
                UI_LOG_ERROR("ERR. Sframe key must be 16 bytes");
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "SFrame key must be 16 bytes");
                break;
            }

            if (storage.Save(ConfigStorage::Config_Id::Sframe_Key, packet->payload.data(),
                             packet->length))
            {
                UI_LOG_INFO("OK! Saved SFrame Key configuration");
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            }
            else
            {
                UI_LOG_ERROR("ERR. Failed to save SFrame Key configuration");
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Failed to save SFrame key");
            }
            break;
        }
        case CtlToUi::GetSframeKey:
        {
            ConfigStorage::Config config = storage.Load(ConfigStorage::Config_Id::Sframe_Key);
            if (config.loaded && config.len == 16)
            {
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::SframeKey),
                             std::span<const uint8_t>(config.buff, config.len));
            }
            else
            {
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "SFrame key not found");
            }
            break;
        }
        case CtlToUi::GetStackInfo:
        {
            stack_debug::StackInfo info = stack_debug::GetStackInfo();
            char json[128];
            int len = snprintf(
                json, sizeof(json),
                "{\"stack_base\":%lu,\"stack_top\":%lu,\"stack_size\":%lu,\"stack_used\":%lu}",
                info.stack_base, info.stack_top, info.stack_size, info.stack_used);
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::StackInfo),
                         std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(json), len));
            break;
        }
        case CtlToUi::RepaintStack:
        {
            stack_debug::RepaintStack();
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            break;
        }
        case CtlToUi::GetLoopback:
        {
            uint8_t mode = static_cast<uint8_t>(UiLoopbackMode::Off);
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Loopback),
                         std::span<const uint8_t>(&mode, 1));
            break;
        }
        case CtlToUi::SetLoopback:
        {
            if (packet->length < 1)
            {
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Missing loopback mode parameter");
                break;
            }
            auto mode = static_cast<UiLoopbackMode>(packet->payload[0]);
            if (mode == UiLoopbackMode::Off)
            {
                mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            }
            else
            {
                UI_LOG_WARN("Loopback mode %d not supported", static_cast<int>(mode));
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Loopback mode not supported");
            }
            break;
        }
        case CtlToUi::GetLogsEnabled:
        {
            uint8_t enabled = Logger::enabled ? 1 : 0;
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::LogsEnabled),
                         std::span<const uint8_t>(&enabled, 1));
            break;
        }
        case CtlToUi::SetLogsEnabled:
        {
            if (packet->length < 1)
            {
                mgmt_serial.ReplyError(static_cast<uint16_t>(UiToCtl::Error), "Missing logs enabled parameter");
                break;
            }
            uint8_t enabled = packet->payload[0];
            if (enabled)
            {
                Logger::Enable();
            }
            else
            {
                Logger::Disable();
            }
            mgmt_serial.Reply(static_cast<uint16_t>(UiToCtl::Ack), std::span<const uint8_t>{});
            break;
        }
        default:
        {
            UI_LOG_ERROR("ERR. No handler for packet type 0x%04x", packet->type);
            break;
        }
        }
    }
}

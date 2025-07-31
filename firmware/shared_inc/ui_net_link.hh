#pragma once

#include "constants.hh"
#include "link_packet_t.hh"
#include <cstring>

namespace ui_net_link
{
enum struct Packet_Type : uint8_t
{
    PowerOnReady,
    GetAudioLinkPacket,
    GetTextLinkPacket,
    TalkStart, // Dead type?
    TalkStop,  // Dead type?
    PlayStart,
    PlayStop,
    MoQStatus,
    MoQChangeNamespace,
    PttObject,
    PttMultiObject,
    PttAiObject,
    AiResponse,
    TextMessage,
    SSIDRequest,
    WifiConnect,
    WifiStatus
};

enum class Channel_Id : uint8_t
{
    Ptt,
    Ptt_Ai,
    Chat,
    Chat_Ai,
    Count
};

struct ChangeNamespace
{

    Channel_Id channel_id;
    uint16_t trackname_len;
    char trackname[128]; // Todo get actual max size of a trackname
};

struct TalkStart
{
    Channel_Id channel_id;
};

struct TalkStop
{
    Channel_Id channel_id;
};

struct PlayStart
{
    Channel_Id channel_id;
};

struct PlayStop
{
    Channel_Id channel_id;
};

struct AudioObject
{
    Channel_Id channel_id;
    uint8_t data[constants::Audio_Phonic_Sz];
};

struct TextObject
{
    Channel_Id channel_id;
    uint16_t length;
    uint8_t data[48]; // TODO get from screen? Or constants fil
};

struct WifiStatus
{
    const uint8_t type = static_cast<uint8_t>(Packet_Type::WifiStatus);
    const PACKET_LENGTH_TYPE len = 1;
    uint8_t status;
};

enum class MessageType : uint8_t
{
    Media = 1,
    AIRequest,
    AIResponse,
    Chat,
};

enum class ContentType : uint8_t
{
    Audio = 0,
    Json,
};

struct __attribute__((packed)) Chunk
{
    const MessageType type = MessageType::Media;
    uint8_t last_chunk;
    std::uint32_t chunk_length;
    std::uint8_t chunk_data[constants::Audio_Phonic_Sz];
};

struct __attribute__((packed)) AIRequestChunk
{
    const MessageType type = MessageType::AIRequest;
    std::uint32_t request_id;
    bool last_chunk;
    std::uint32_t chunk_length;
    std::uint8_t chunk_data[constants::Audio_Phonic_Sz];
};

struct __attribute__((packed)) AIResponseChunk
{
    const MessageType type = MessageType::AIResponse;
    std::uint32_t request_id;
    ContentType content_type;
    bool last_chunk;
    std::uint32_t chunk_length;

    static constexpr uint32_t Type_Size = sizeof(type);
    static constexpr uint32_t Response_Id_Size = sizeof(request_id);
    static constexpr uint32_t Content_Type_Size = sizeof(content_type);
    static constexpr uint32_t Last_Chunk_Size = sizeof(last_chunk);
    static constexpr uint32_t Chunk_Length_Size = sizeof(chunk_length);

    // Ext bytes that are always in packet
    static constexpr uint32_t Channel_Id_Size = sizeof(Channel_Id);

    static constexpr uint32_t Chunk_Size =
        link_packet_t::Payload_Size
        - (Type_Size + Response_Id_Size + Content_Type_Size + Last_Chunk_Size + Chunk_Length_Size
           + Channel_Id_Size);
    std::uint8_t chunk_data[Chunk_Size];
};

// example
// auto* response =
// static_cast<ui_net_link::AIResponseChunk*>(static_cast<void*>(packet->payload + 1));

[[maybe_unused]] static void BuildGetLinkPacket(uint8_t* buff)
{
    buff[0] = (uint8_t)Packet_Type::GetAudioLinkPacket;
    buff[1] = 0;
    buff[2] = 0;
}

[[maybe_unused]] static void Serialize(const WifiStatus& wifi_status, link_packet_t& packet)
{
    packet.type = wifi_status.type;
    packet.length = wifi_status.len;
    packet.payload[0] = (uint8_t)wifi_status.status;
    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const TalkStart& talk_start, link_packet_t& packet)
{
    packet.type = static_cast<uint8_t>(Packet_Type::TalkStart);
    packet.length = 1;

    // Channel id
    packet.payload[0] = static_cast<uint8_t>(talk_start.channel_id);

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const TalkStop& talk_stop, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::TalkStop;
    packet.length = 1;

    // Channel id
    packet.payload[0] = (uint8_t)talk_stop.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const PlayStart& play_start, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::PlayStart;
    packet.length = 1;

    // Channel id
    packet.payload[0] = (uint8_t)play_start.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const PlayStop& play_stop, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::PlayStop;
    packet.length = 1;

    // Channel id
    packet.payload[0] = (uint8_t)play_stop.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const AudioObject& talk_frame,
                                       Packet_Type packet_type,
                                       bool is_last,
                                       link_packet_t& packet)
{
    if (packet_type != Packet_Type::PttObject && packet_type != Packet_Type::PttAiObject)
    {
        // This should maybe be an error instead?
        Logger::Log(Logger::Level::Error, "audio object packet type is a wrong type %d",
                    (int)packet_type);
        packet_type = Packet_Type::PttObject;
    }

    packet.type = (uint8_t)packet_type;
    packet.payload[0] = (uint8_t)talk_frame.channel_id;

    uint32_t offset = 1;

    static constexpr std::uint32_t audio_size = constants::Audio_Phonic_Sz;
    if (packet_type == Packet_Type::PttObject)
    {
        packet.payload[offset] = static_cast<uint8_t>(MessageType::Media);
        offset += sizeof(Chunk::type);

        packet.payload[offset] = static_cast<uint8_t>(is_last);
        offset += sizeof(bool);

        memcpy(packet.payload + offset, &audio_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    else if (packet_type == Packet_Type::PttAiObject)
    {
        packet.payload[offset] = static_cast<uint8_t>(MessageType::AIRequest);
        offset += sizeof(Chunk::type);

        // TODO: We don't use this now but in future we should have this provided from somewhere.
        uint32_t request_id = 0;
        memcpy(packet.payload + offset, &request_id, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        packet.payload[offset] = static_cast<uint8_t>(is_last);
        offset += sizeof(bool);

        memcpy(packet.payload + offset, &audio_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    packet.length = offset + constants::Audio_Phonic_Sz;

    memcpy(packet.payload + offset, talk_frame.data, constants::Audio_Phonic_Sz);

    packet.is_ready = true;
}

[[maybe_unused]] static void
Serialize(const Channel_Id channel_id, const char* text, const uint32_t len, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::TextMessage;
    packet.payload[0] = (uint8_t)channel_id;

    uint32_t offset = 1;
    packet.payload[offset] = static_cast<uint8_t>(MessageType::Chat);
    offset += sizeof(MessageType);

    memcpy(packet.payload + offset, &len, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(packet.payload + offset, text, len);

    packet.length = offset + len;
    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const ChangeNamespace& change_moq_namespace_frame,
                                       link_packet_t& packet)
{
    const uint16_t num_extra_bytes = sizeof(change_moq_namespace_frame.channel_id)
                                   + sizeof(change_moq_namespace_frame.trackname_len);
    uint32_t payload_offset = 0;

    packet.type = (uint8_t)Packet_Type::MoQChangeNamespace;
    packet.length = num_extra_bytes + change_moq_namespace_frame.trackname_len;

    memcpy(packet.payload, &change_moq_namespace_frame.channel_id,
           sizeof(change_moq_namespace_frame.channel_id));
    payload_offset += sizeof(change_moq_namespace_frame.channel_id);

    memcpy(packet.payload + payload_offset, &change_moq_namespace_frame.trackname_len,
           sizeof(change_moq_namespace_frame.trackname_len));
    payload_offset += sizeof(change_moq_namespace_frame.trackname_len);

    memcpy(packet.payload + payload_offset, change_moq_namespace_frame.trackname,
           change_moq_namespace_frame.trackname_len);

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const uint8_t* ssid,
                                       const uint16_t ssid_len,
                                       const uint8_t* pwd,
                                       const uint16_t pwd_len,
                                       link_packet_t& packet)
{
    const uint16_t num_extra_bytes = sizeof(ssid_len) + sizeof(pwd_len);
    size_t payload_offset = 0;

    packet.type = (uint8_t)Packet_Type::WifiConnect;
    packet.length = ssid_len + pwd_len + num_extra_bytes;

    memcpy(packet.payload, &ssid_len, sizeof(ssid_len));
    payload_offset += sizeof(ssid_len);
    memcpy(packet.payload + payload_offset, ssid, ssid_len);
    payload_offset += ssid_len;

    memcpy(packet.payload + payload_offset, &pwd_len, sizeof(pwd_len));
    payload_offset += sizeof(pwd_len);
    memcpy(packet.payload + payload_offset, pwd, pwd_len);
    payload_offset += pwd_len;

    packet.is_ready = true;
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStart& talk_start)
{
    talk_start.channel_id = (Channel_Id)packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStop& talk_stop)
{
    talk_stop.channel_id = (Channel_Id)packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStart& play_start)
{
    play_start.channel_id = (Channel_Id)packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStop& play_stop)
{
    play_stop.channel_id = (Channel_Id)packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, AudioObject& audio_object)
{
    audio_object.channel_id = (Channel_Id)packet.payload[0];

    uint32_t payload_offset = 1 + sizeof(Chunk) - constants::Audio_Phonic_Sz;
    if (static_cast<MessageType>(packet.payload[1]) == MessageType::AIResponse)
    {
        payload_offset = 1 + sizeof(AIResponseChunk) - constants::Audio_Phonic_Sz;
    }

    memcpy(audio_object.data, packet.payload + payload_offset, constants::Audio_Phonic_Sz);
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet,
                                         ChangeNamespace& change_moq_namespace_frame)
{
    memcpy(&change_moq_namespace_frame.channel_id, packet.payload,
           sizeof(change_moq_namespace_frame.channel_id));
    uint32_t payload_offset = sizeof(change_moq_namespace_frame.channel_id);

    memcpy(&change_moq_namespace_frame.trackname_len, packet.payload + payload_offset,
           sizeof(change_moq_namespace_frame.trackname_len));
    payload_offset += sizeof(change_moq_namespace_frame.trackname_len);

    memcpy(change_moq_namespace_frame.trackname, packet.payload + payload_offset,
           change_moq_namespace_frame.trackname_len);
}

[[maybe_unused]] static void
Deserialize(const link_packet_t& packet, std::string& ssid, std::string& pwd)
{
    size_t payload_offset = 0;

    uint16_t ssid_len = 0;
    uint16_t pwd_len = 0;

    memcpy(&ssid_len, packet.payload + payload_offset, sizeof(ssid_len));
    payload_offset += sizeof(ssid_len);
    ssid.resize(ssid_len);
    memcpy(ssid.data(), packet.payload, ssid_len);
    payload_offset += ssid_len;

    memcpy(&pwd_len, packet.payload + payload_offset, sizeof(pwd_len));
    payload_offset += sizeof(pwd_len);
    pwd.resize(pwd_len);
    memcpy(pwd.data(), packet.payload + payload_offset, pwd_len);
    payload_offset += pwd_len;
}

// TODO serialize and deserialize for audioobjectS
} // namespace ui_net_link
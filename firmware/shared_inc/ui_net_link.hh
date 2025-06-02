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
    PttAIObject,
    TextMessage,
    SSIDRequest,
    WifiConnect,
    WifiStatus
};

struct ChangeNamespace
{

    uint8_t channel_id;
    uint16_t trackname_len;
    char trackname[128]; // Todo get actual max size of a trackname
};

struct TalkStart
{
    uint8_t channel_id;
};

struct TalkStop
{
    uint8_t channel_id;
};

struct PlayStart
{
    uint8_t channel_id;
};

struct PlayStop
{
    uint8_t channel_id;
};

struct AudioObject
{
    uint8_t channel_id;
    uint8_t data[constants::Audio_Phonic_Sz];
};

struct TextObject
{
    uint8_t channel_id;
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
    AIResponse
};

enum class ContentType : uint8_t
{
    Audio = 0,
    Json,
};

struct __attribute__((packed)) Chunk
{
    // const MessageType type = MessageType::Media;
    const uint8_t type = (uint8_t)MessageType::Media;
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
    std::uint8_t chunk_data[constants::Audio_Phonic_Sz];
};

static_assert(sizeof(Chunk) == 166);
static_assert(sizeof(AIRequestChunk) == 170);
static_assert(sizeof(AIResponseChunk) == 171);

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
    packet.payload[0] = wifi_status.status;
    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const TalkStart& talk_start, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::TalkStart;
    packet.length = 1;

    // Channel id
    packet.payload[0] = talk_start.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const TalkStop& talk_stop, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::TalkStop;
    packet.length = 1;

    // Channel id
    packet.payload[0] = talk_stop.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const PlayStart& play_start, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::PlayStart;
    packet.length = 1;

    // Channel id
    packet.payload[0] = play_start.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const PlayStop& play_stop, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::PlayStop;
    packet.length = 1;

    // Channel id
    packet.payload[0] = play_stop.channel_id;

    packet.is_ready = true;
}

[[maybe_unused]] static void Serialize(const AudioObject& talk_frame,
                                       Packet_Type packet_type,
                                       bool is_last,
                                       link_packet_t& packet)
{
    if (packet_type != Packet_Type::PttObject && packet_type != Packet_Type::PttAIObject)
    {
        packet_type = Packet_Type::PttObject;
    }

    packet.type = (uint8_t)packet_type;
    packet.payload[0] = talk_frame.channel_id;

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
    else if (packet_type == Packet_Type::PttAIObject)
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
Serialize(const uint8_t channel_id, const char* text, const uint16_t len, link_packet_t& packet)
{
    packet.type = (uint8_t)Packet_Type::TextMessage;
    packet.payload[0] = channel_id;

    memcpy(packet.payload + 1, text, len);

    packet.length = 1 + len;
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

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStart& talk_start)
{
    talk_start.channel_id = packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, TalkStop& talk_stop)
{
    talk_stop.channel_id = packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStart& play_start)
{
    play_start.channel_id = packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, PlayStop& play_stop)
{
    play_stop.channel_id = packet.payload[0];
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, AudioObject& audio_object)
{
    audio_object.channel_id = packet.payload[0];

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

// TODO serialize and deserialize for audioobjectS
} // namespace ui_net_link
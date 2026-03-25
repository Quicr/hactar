#pragma once

#include "constants.hh"
#include "link_packet_t.hh"
#include <cstdint>
#include <cstring>
#include <span>

namespace ui_net_link
{
enum struct Packet_Type : uint16_t
{
    Message = 0,
};

enum class Channel_Id : uint8_t
{
    Ptt,
    Ptt_Ai,
    Chat,
    Chat_Ai,
    Count
};

struct AudioObject
{
    Channel_Id channel_id;
    uint8_t data[constants::Audio_Phonic_Sz];
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

[[maybe_unused]] static void Serialize(const AudioObject& talk_frame,
                                       bool is_last,
                                       link_packet_t& packet)
{
    packet.type = (uint16_t)Packet_Type::Message;
    packet.payload[0] = (uint8_t)talk_frame.channel_id;

    uint32_t offset = 1;

    static constexpr std::uint32_t audio_size = constants::Audio_Phonic_Sz;
    if (talk_frame.channel_id == Channel_Id::Ptt)
    {
        packet.payload[offset] = static_cast<uint8_t>(MessageType::Media);
        offset += sizeof(Chunk::type);

        packet.payload[offset] = static_cast<uint8_t>(is_last);
        offset += sizeof(bool);

        memcpy(packet.payload.data() + offset, &audio_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }
    else if (talk_frame.channel_id == Channel_Id::Ptt_Ai)
    {
        packet.payload[offset] = static_cast<uint8_t>(MessageType::AIRequest);
        offset += sizeof(Chunk::type);

        // TODO: We don't use this now but in future we should have this provided from somewhere.
        uint32_t request_id = 0;
        memcpy(packet.payload.data() + offset, &request_id, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        packet.payload[offset] = static_cast<uint8_t>(is_last);
        offset += sizeof(bool);

        memcpy(packet.payload.data() + offset, &audio_size, sizeof(uint32_t));
        offset += sizeof(uint32_t);
    }

    packet.length = offset + constants::Audio_Phonic_Sz;

    memcpy(packet.payload.data() + offset, talk_frame.data, constants::Audio_Phonic_Sz);

    packet.is_ready = true;
}

[[maybe_unused]] static void
Serialize(const Channel_Id channel_id, const char* text, const uint32_t len, link_packet_t& packet)
{
    packet.type = (uint16_t)Packet_Type::Message;
    packet.payload[0] = (uint8_t)channel_id;

    uint32_t offset = 1;
    packet.payload[offset] = static_cast<uint8_t>(MessageType::Chat);
    offset += sizeof(MessageType);

    memcpy(packet.payload.data() + offset, &len, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    memcpy(packet.payload.data() + offset, text, len);

    packet.length = offset + len;
    packet.is_ready = true;
}

[[maybe_unused]] static void Deserialize(const link_packet_t& packet, AudioObject& audio_object)
{
    audio_object.channel_id = (Channel_Id)packet.payload[0];

    uint32_t payload_offset = 1 + sizeof(Chunk) - constants::Audio_Phonic_Sz;
    if (static_cast<MessageType>(packet.payload[1]) == MessageType::AIResponse)
    {
        payload_offset = 1 + sizeof(AIResponseChunk) - constants::Audio_Phonic_Sz;
    }

    memcpy(audio_object.data, packet.payload.data() + payload_offset, constants::Audio_Phonic_Sz);
}

} // namespace ui_net_link

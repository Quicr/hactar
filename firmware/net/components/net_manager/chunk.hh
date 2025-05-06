#pragma once

namespace moq
{
enum class MessageType : uint8_t
{
    Media = 1,
    AIRequest,
    AIResponse
};

enum class Content_Type
{
    Audio = 0,
    Json,
};

struct Chunk
{
    const MessageType type = MessageType::Media;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};

struct AIRequestChunk
{
    const MessageType type = MessageType::AIRequest;
    std::uint32_t request_id;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};

struct AIResponseChunk
{
    const MessageType type = MessageType::AIResponse;
    std::uint32_t request_id;
    Content_Type content_type;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};
} // namespace moq
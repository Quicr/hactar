#pragma once

#include <stdint.h>
#include <optional>
#include <vector>
#include <cstring>

struct Chunk
{
public:
    enum class ContentType:uint8_t
    {
        Audio = 0,
        Json,
    };

    ContentType type;
    bool is_last;
    uint32_t length;
    std::vector<uint8_t> data;

    static Chunk Deserialize(std::optional<std::vector<uint8_t>>& bytes)
    {
        Chunk chunk;
        uint32_t offset = 0;
        chunk.type = (ContentType)bytes->at(offset++);
        chunk.is_last = bytes->at(offset++);

        std::memcpy(&chunk.length, bytes->data() + offset, sizeof(chunk.length));
        offset += sizeof(chunk.length);

        chunk.data.assign(bytes->data() + offset, bytes->data() + chunk.length + offset);

        return chunk;
    }

    static void Serialize()
    {

    }
};
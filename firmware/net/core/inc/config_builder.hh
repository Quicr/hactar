#pragma once

#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using json = nlohmann::json;

// Supported language tags
inline constexpr std::array<const char*, 5> kSupportedLanguages = {
    "en-US", "es-ES", "de-DE", "hi-IN", "nb-NO",
};

inline bool IsValidLanguage(const std::string& lang)
{
    return std::find(kSupportedLanguages.begin(), kSupportedLanguages.end(), lang)
        != kSupportedLanguages.end();
}

// Encode a namespace (vector of strings) to wire format:
// [4 bytes: num_parts] [for each part: 4 bytes length + UTF-8 string]
inline std::vector<uint8_t> EncodeNamespace(const std::vector<std::string>& ns)
{
    std::vector<uint8_t> result;
    uint32_t num_parts = static_cast<uint32_t>(ns.size());
    result.insert(result.end(), reinterpret_cast<uint8_t*>(&num_parts),
                  reinterpret_cast<uint8_t*>(&num_parts) + sizeof(num_parts));

    for (const auto& part : ns)
    {
        uint32_t len = static_cast<uint32_t>(part.size());
        result.insert(result.end(), reinterpret_cast<uint8_t*>(&len),
                      reinterpret_cast<uint8_t*>(&len) + sizeof(len));
        result.insert(result.end(), part.begin(), part.end());
    }
    return result;
}

// Decode a namespace from wire format, returns bytes consumed
inline size_t DecodeNamespace(const uint8_t* data, size_t max_len, std::vector<std::string>& ns)
{
    ns.clear();
    if (max_len < sizeof(uint32_t))
    {
        return 0;
    }

    size_t offset = 0;
    uint32_t num_parts = 0;
    memcpy(&num_parts, data + offset, sizeof(num_parts));
    offset += sizeof(num_parts);

    for (uint32_t i = 0; i < num_parts && offset < max_len; ++i)
    {
        if (offset + sizeof(uint32_t) > max_len)
        {
            break;
        }
        uint32_t len = 0;
        memcpy(&len, data + offset, sizeof(len));
        offset += sizeof(len);

        if (offset + len > max_len)
        {
            break;
        }
        ns.emplace_back(reinterpret_cast<const char*>(data + offset), len);
        offset += len;
    }
    return offset;
}

// Convert namespace to/from JSON for storage
inline std::string NamespaceToJson(const std::vector<std::string>& ns)
{
    json j = ns;
    return j.dump();
}

inline std::vector<std::string> JsonToNamespace(const std::string& json_str)
{
    if (json_str.empty())
    {
        return {};
    }
    try
    {
        json j = json::parse(json_str);
        return j.get<std::vector<std::string>>();
    }
    catch (...)
    {
        return {};
    }
}

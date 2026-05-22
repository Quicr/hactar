#pragma once

#include "storage.hh"
#include "stored_value.hh"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

using json = nlohmann::json;

struct ConfigState
{
public:
    StoredValue<std::string> moq_server_url;
    StoredValue<std::string> language;
    StoredValue<std::string> channel_ns_json;
    StoredValue<std::string> ai_query_ns_json;
    StoredValue<std::string> ai_audio_response_ns_json;
    StoredValue<std::string> ai_cmd_response_ns_json;

    // In-memory namespace caches (decoded from JSON)
    std::vector<std::string> channel_ns;
    std::vector<std::string> ai_query_ns;
    std::vector<std::string> ai_audio_response_ns;
    std::vector<std::string> ai_cmd_response_ns;

    ConfigState(Storage& storage) :
        moq_server_url(storage, "moq", "server_url"),
        language(storage, "config", "language"),
        channel_ns_json(storage, "config", "channel_ns"),
        ai_query_ns_json(storage, "config", "ai_qry_ns"),
        ai_audio_response_ns_json(storage, "config", "ai_aud_ns"),
        ai_cmd_response_ns_json(storage, "config", "ai_cmd_ns"),
        channel_ns(),
        ai_query_ns(),
        ai_audio_response_ns(),
        ai_cmd_response_ns()

    {
    }

    // Supported language tags
    static inline constexpr std::array<const char*, 5> kSupportedLanguages = {
        "en-US", "es-ES", "de-DE", "hi-IN", "nb-NO",
    };

    static inline bool IsValidLanguage(const std::string& lang)
    {
        return std::find(kSupportedLanguages.begin(), kSupportedLanguages.end(), lang)
            != kSupportedLanguages.end();
    }

    // Parse namespace from JSON string
    // Returns std::nullopt if parsing fails or string is empty
    static inline std::optional<std::vector<std::string>>
    JsonToNamespace(const std::string& json_str)
    {
        if (json_str.empty() || !json::accept(json_str))
        {
            return std::nullopt;
        }

        json j = json::parse(json_str, nullptr, false);
        if (j.is_discarded() || !j.is_array())
        {
            return std::nullopt;
        }

        try
        {
            return j.get<std::vector<std::string>>();
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    // Load namespaces from storage into memory
    // Initialize empty entries to "[]" so storage always contains valid JSON
    void LoadNamespacesFromStorage()
    {
        auto load_ns = [](StoredValue<std::string>& stored, std::vector<std::string>& ns) {
            std::string json_str = stored.Load();
            if (json_str.empty())
            {
                stored = "[]";
                ns.clear();
                return;
            }
            auto parsed = ConfigState::JsonToNamespace(json_str);
            ns = parsed.value_or(std::vector<std::string>{});
        };

        load_ns(channel_ns_json, channel_ns);
        load_ns(ai_query_ns_json, ai_query_ns);
        load_ns(ai_audio_response_ns_json, ai_audio_response_ns);
        load_ns(ai_cmd_response_ns_json, ai_cmd_response_ns);
    }
};

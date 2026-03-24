#pragma once

#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <optional>
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

// Convert namespace to JSON string
inline std::string NamespaceToJson(const std::vector<std::string>& ns)
{
    return json(ns).dump();
}

// Parse namespace from JSON string
// Returns std::nullopt if parsing fails or string is empty
inline std::optional<std::vector<std::string>> JsonToNamespace(const std::string& json_str)
{
    if (json_str.empty())
    {
        return std::nullopt;
    }
    try
    {
        json j = json::parse(json_str);
        if (!j.is_array())
        {
            return std::nullopt;
        }
        return j.get<std::vector<std::string>>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

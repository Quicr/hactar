#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

using json = nlohmann::json;

class ChannelBuilder
{
public:
    ChannelBuilder(const std::vector<std::string>& track_ns_prefix, std::uint64_t endpoint_id) :
        _track_ns_prefix(track_ns_prefix)
    {
        _config = json{
            {
                "endpoint_info",
                {
                    {"id", "c637ae2a-b3d7-4574-bac7-8c7b5ee44f3e"},
                    {"owner", "alice@acme.com"},
                    {"lang-preference", ""},
                },
            },
        };

        _config["publications"] = json::array();
        _config["subscriptions"] = json::array();

        json ai_audio_channel{
            {"channel_name", "self_ai_audio"},
            {"tracknamespace",
             std::vector<std::string>{"cisco.com", "q_ai", "5ea0fe26-01b7-4852-a833-75f3868f139f",
                                      "responses"}},
            {"trackname", std::to_string(endpoint_id)},
            {"codec", "pcm"},
            {"samplerate", 8000},
        };

        json ai_text_channel{
            {"channel_name", "self_ai_text"},
            {"tracknamespace",
             std::vector<std::string>{"cisco.com", "q_ai", "5ea0fe26-01b7-4852-a833-75f3868f139f",
                                      "response_command"}},
            {"trackname", std::to_string(endpoint_id)},
            {"codec", "ai_cmd_response:json"},
        };

        _config["subscriptions"].push_back(ai_audio_channel);
        _config["subscriptions"].push_back(ai_text_channel);
    }

    ChannelBuilder& AddPublicationChannel(const std::string& name,
                                          const std::string& language,
                                          const std::string& codec)
    {
        _config["publications"].push_back(BuildChannel(_track_ns_prefix, name, language, codec));

        return *this;
    }

    ChannelBuilder& AddSubscriptionChannel(const std::string& name,
                                           const std::string& language,
                                           const std::string& codec)
    {
        _config["subscriptions"].push_back(BuildChannel(_track_ns_prefix, name, language, codec));

        return *this;
    }

    ChannelBuilder& AddAIAudioPublicationChannel(const std::string& language)
    {
        _config["publications"].push_back(
            BuildChannel({"cisco.com", "q_ai", "5ea0fe26-01b7-4852-a833-75f3868f139f", "questions"},
                         "ai_audio", language, "pcm"));

        return *this;
    }

    const json& GetConfig() const
    {
        return _config;
    }

protected:
    json BuildChannel(const std::vector<std::string>& track_ns,
                      const std::string& name,
                      const std::string& language,
                      const std::string& codec)
    {
        json channel;
        channel["channel_name"] = name;
        channel["language"] = language;
        channel["tracknamespace"] = track_ns;
        channel["codec"] = codec;

        if (codec == "pcm")
        {
            channel["trackname"] = language;
            channel["samplerate"] = 8000;
        }
        else
        {
            channel["trackname"] = std::format("chat_{}", language.substr(0, 2));
        }

        return channel;
    }

private:
    const std::vector<std::string> _track_ns_prefix;
    json _config;
};

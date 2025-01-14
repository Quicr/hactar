// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __NET_UTILS__
#define __NET_UTILS__

#include <sstream>
#include <string>
#include <iomanip>

#include <quicr/track_name.h>

namespace moq {

static quicr::FullTrackName const MakeFullTrackName(const std::string& track_namespace,
                                                    const std::string& track_name,
                                                    uint64_t track_alias) noexcept {
const auto split = [](std::string str, const std::string& delimiter) {
    std::vector<std::string> tokens;

    std::size_t pos = 0;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        tokens.emplace_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }
    tokens.emplace_back(std::move(str));

    return tokens;
};

quicr::FullTrackName full_track_name{ quicr::TrackNamespace{ split(track_namespace, ",") },
                                      { track_name.begin(), track_name.end() },
                                      track_alias };
return full_track_name;
}

static std::string Stringify(const quicr::FullTrackName& ftn)
{
    std::stringstream ss;
    ss << "[FTN=";
    const auto& entries = ftn.name_space.GetEntries();
    for (const auto& entry : Span{ entries }.subspan(0, ftn.name_space.GetEntries().size() - 1)) {
        ss << std::string(entry.begin(), entry.end()) << "/";
    }
    ss << std::string(entries.back().begin(), entries.back().end());
    ss << "/" << std::string(ftn.name.begin(), ftn.name.end());
    if (ftn.track_alias) {
        ss << ";Alias=" << ftn.track_alias.value();
    }
    ss << "]";
    return ss.str();
}


static std::string
to_hex(const std::vector<uint8_t> & data)
{
    std::stringstream hex(std::ios_base::out);
    hex.flags(std::ios::hex);
    for (const auto& byte : data) {
        hex << std::setw(2) << std::setfill('0') << int(byte);
    }
    return hex.str();
}

static quicr::FullTrackName const
MakeFullTrackName(const std::string& track_namespace,
                  const std::string& track_name,
                  const std::optional<uint64_t> track_alias)
{
    const auto split= [](std::string str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    while ((pos = str.find(delimiter)) != std::string::npos) {
        tokens.emplace_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }

        tokens.emplace_back(std::move(str));
        return tokens;
    };

    quicr::FullTrackName full_track_name{ quicr::TrackNamespace{ split(track_namespace, "/") },
                                          { track_name.begin(), track_name.end() }, track_alias };
    return full_track_name;
}


}

#endif
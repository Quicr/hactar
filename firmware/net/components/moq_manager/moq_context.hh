#pragma once

#include "config_state.hh"
#include "moq_session.hh"
#include "net.hh"
#include "serial.hh"
#include <memory>
#include <vector>

class MoqContext
{
public:
    MoqContext(Serial& ui_layer, const Runtime& runtime, const Diagnostics& diagnostics);

    void InitializeSession(const quicr::ClientConfig& config);
    void StopSession();
    void RestartSession(const quicr::ClientConfig& config);

    moq::Session::Status GetStatus() const;
    bool Connect();

    void UpdateAITracks(ConfigState& config);
    void UpdateChannelTracks(ConfigState& config);
    void StartTracks();

    void
    PushAudioFrame(uint8_t channel_id, const uint8_t* payload, uint32_t length, uint64_t timestamp);

private:
    std::shared_ptr<moq::TrackReader>
    CreateReadTrack(const std::string& channel_name,
                    const std::vector<std::string>& track_namespace,
                    const std::string& trackname,
                    const std::string& codec);
    std::shared_ptr<moq::TrackWriter>
    CreateWriteTrack(const std::string& channel_name,
                     const std::vector<std::string>& track_namespace,
                     const std::string& trackname,
                     const std::string& codec);

    Serial& ui_layer;
    const Runtime& runtime;
    const Diagnostics& diagnostics;
    std::vector<std::shared_ptr<moq::TrackReader>> readers;
    std::vector<std::shared_ptr<moq::TrackWriter>> writers;
    std::shared_ptr<moq::Session> session;
};

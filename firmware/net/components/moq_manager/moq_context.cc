#include "moq_context.hh"
#include "logger.hh"
#include "peripherals.hh"
#include "utils.hh"

MoqContext::MoqContext(Serial& ui_layer, const Runtime& runtime, const Diagnostics& diagnostics) :
    ui_layer(ui_layer),
    runtime(runtime),
    diagnostics(diagnostics),
    readers(static_cast<uint32_t>(ui_net_link::Channel_Id::Count)),
    writers(static_cast<uint32_t>(ui_net_link::Channel_Id::Count) - 1),
    session(nullptr)
{
}

void MoqContext::InitializeSession(const quicr::ClientConfig& config)
{
    session.reset(new moq::Session(config, readers, writers));
}

void MoqContext::StopSession()
{
    if (!session)
    {
        return;
    }

    session->StopTracks();
    session->Disconnect();

    for (const auto& reader : readers)
    {
        if (reader)
        {
            reader->Stop();
        }
    }

    for (const auto& writer : writers)
    {
        if (writer)
        {
            writer->Stop();
        }
    }

    gpio_set_level(NET_LED_B, 1);
}

void MoqContext::RestartSession(const quicr::ClientConfig& config)
{
    StopSession();
    InitializeSession(config);
}

moq::Session::Status MoqContext::GetStatus() const
{
    if (!session)
    {
        return moq::Session::Status::kNotConnected;
    }

    return session->GetStatus();
}

bool MoqContext::Connect()
{
    if (!session)
    {
        NET_LOG_WARN("MoQ session not ready, cannot connect");
        return false;
    }

    return session->Connect() == quicr::Transport::Status::kConnecting;
}

void MoqContext::UpdateAITracks(ConfigState& config)
{
    const std::string lang = config.language.Load();
    const std::string device_id_str = std::to_string(runtime.device_id);

    if (!config.ai_query_ns.empty() && !lang.empty())
    {
        if (auto writer = CreateWriteTrack("ai_audio", config.ai_query_ns, lang, "pcm", config))
        {
            writer->Start();
        }
    }

    if (!config.ai_audio_response_ns.empty())
    {
        if (auto reader =
                CreateReadTrack("self_ai_audio", config.ai_audio_response_ns, device_id_str, "pcm"))
        {
            reader->Start();
        }
    }

    if (!config.ai_cmd_response_ns.empty())
    {
        if (auto reader = CreateReadTrack("self_ai_text", config.ai_cmd_response_ns, device_id_str,
                                          "ai_cmd_response:json"))
        {
            reader->Start();
        }
    }
}

void MoqContext::UpdateChannelTracks(ConfigState& config)
{
    const std::string lang = config.language.Load();

    if (config.channel_ns.empty() || lang.empty())
    {
        NET_LOG_WARN("Cannot update channel tracks: namespace or language empty");
        return;
    }

    if (auto writer = CreateWriteTrack("channel", config.channel_ns, lang, "pcm", config))
    {
        writer->Start();
    }

    if (auto reader = CreateReadTrack("channel", config.channel_ns, lang, "pcm"))
    {
        reader->Start();
    }
}

void MoqContext::StartTracks()
{
    for (const auto& reader : readers)
    {
        if (reader)
        {
            reader->Start();
        }
    }

    for (const auto& writer : writers)
    {
        if (writer)
        {
            writer->Start();
        }
    }
}

void MoqContext::PushAudioFrame(uint8_t channel_id,
                                const uint8_t* payload,
                                uint32_t length,
                                uint64_t timestamp)
{
    if (channel_id >= writers.size())
    {
        NET_LOG_ERROR("Channel id received for a writer that doesn't exist!");
        return;
    }

    if (writers[channel_id])
    {
        writers[channel_id]->PushObject(payload, length, timestamp);
    }
}

std::shared_ptr<moq::TrackReader>
MoqContext::CreateReadTrack(const std::string& channel_name,
                            const std::vector<std::string>& track_namespace,
                            const std::string& trackname,
                            const std::string& codec)
try
{
    uint32_t offset = 0;
    if (codec == "pcm")
    {
        offset = channel_name == "self_ai_audio" ? (uint32_t)ui_net_link::Channel_Id::Ptt_Ai
                                                 : (uint32_t)ui_net_link::Channel_Id::Ptt;
    }
    else if (codec == "ascii")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat;
    }
    else if (codec == "ai_cmd_response:json")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat_Ai;
    }
    else
    {
        NET_LOG_ERROR("Unknown reader channel %s", codec.c_str());
        return nullptr;
    }

    if (!session)
    {
        NET_LOG_WARN("MoQ session not ready, cannot create reader");
        return nullptr;
    }

    const auto desired_ftn = moq::MakeFullTrackName(track_namespace, trackname);

    std::unique_lock<std::mutex> lock = session->GetReaderLock();
    if (readers[offset] != nullptr)
    {
        const auto existing_ftn = readers[offset]->GetFullTrackName();
        if (existing_ftn.name_space == desired_ftn.name_space
            && existing_ftn.name == desired_ftn.name)
        {
            NET_LOG_INFO("Reader on %d already exists with same track, reusing", offset);
            return readers[offset];
        }
        NET_LOG_WARN("Reader on %d already exists with different track, replacing", offset);
        readers[offset]->Stop();
        session->UnsubscribeTrack(readers[offset]);
    }

    NET_LOG_WARN("Create reader %s:%s idx %d", channel_name.c_str(), codec.c_str(), offset);
    readers[offset].reset(new moq::TrackReader(desired_ftn, ui_layer, codec, runtime, diagnostics));

    lock.unlock();
    return readers[offset];
}
catch (const std::exception& ex)
{
    NET_LOG_ERROR("Exception in sub %s", ex.what());
    return nullptr;
}

std::shared_ptr<moq::TrackWriter>
MoqContext::CreateWriteTrack(const std::string& channel_name,
                             const std::vector<std::string>& track_namespace,
                             const std::string& trackname,
                             const std::string& codec,
                             const ConfigState& config)
try
{
    if (!session)
    {
        NET_LOG_WARN("MoQ session not ready, cannot create writer");
        return nullptr;
    }

    uint32_t offset = 0;
    if (codec == "pcm")
    {
        offset = channel_name == "ai_audio" ? (uint32_t)ui_net_link::Channel_Id::Ptt_Ai
                                            : (uint32_t)ui_net_link::Channel_Id::Ptt;
    }
    else if (codec == "ascii")
    {
        offset = (uint32_t)ui_net_link::Channel_Id::Chat;
    }
    else
    {
        NET_LOG_ERROR("Unknown writer channel");
        return nullptr;
    }

    const auto desired_ftn = moq::MakeFullTrackName(track_namespace, trackname);

    std::unique_lock<std::mutex> lock = session->GetWriterLock();
    if (writers[offset] != nullptr)
    {
        const auto existing_ftn = writers[offset]->GetFullTrackName();
        if (existing_ftn.name_space == desired_ftn.name_space
            && existing_ftn.name == desired_ftn.name)
        {
            NET_LOG_INFO("Writer on %d already exists with same track, reusing", offset);
            return writers[offset];
        }
        NET_LOG_WARN("Writer on %d already exists with different track, replacing", offset);
        writers[offset]->Stop();
        session->UnpublishTrack(writers[offset]);
    }

    NET_LOG_WARN("Create writer %s:%s idx %d", channel_name.c_str(), codec.c_str(), offset);
    writers[offset].reset(
        new moq::TrackWriter(desired_ftn, quicr::TrackMode::kDatagram, 2, 100, config, runtime));

    lock.unlock();
    return writers[offset];
}
catch (const std::exception& ex)
{
    NET_LOG_ERROR("Exception in pub %s", ex.what());
    return nullptr;
}

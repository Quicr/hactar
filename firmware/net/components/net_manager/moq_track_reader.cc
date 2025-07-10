// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_reader.hh"
#include "chunk.hh"
#include "link_packet_t.hh"
#include "logger.hh"
#include "macros.hh"
#include "moq_session.hh"
#include "task_helpers.hh"
#include "ui_net_link.hh"
#include "utils.hh"

using namespace moq;

extern uint64_t device_id;
extern bool loopback;
extern SemaphoreHandle_t audio_req_smpr;

TrackReader::TrackReader(const quicr::FullTrackName& full_track_name,
                         Serial& serial,
                         const std::string& codec) :
    SubscribeTrackHandler(full_track_name,
                          3,
                          quicr::messages::GroupOrder::kAscending,
                          quicr::messages::FilterType::kLatestObject),
    serial(serial),
    codec(codec),
    track_name(std::string(full_track_name.name_space.begin(), full_track_name.name_space.end())
               + std::string(full_track_name.name.begin(), full_track_name.name.end())),
    byte_buffer(),
    audio_playing(false),
    audio_min_depth(5),
    audio_max_depth(std::numeric_limits<size_t>::max()),
    task_handle(nullptr),
    task_buffer(nullptr),
    task_stack(nullptr),
    num_print(0),
    num_recv(0),
    is_running(true)
{
    task_helpers::Start_PSRAM_Task(SubscribeTask, this, track_name, task_handle, task_buffer,
                                   &task_stack, 8192, 10);
}

// TODO use a notifier?
void TrackReader::ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{
    ++num_print;

    if ((codec == "pcm" && num_print >= 20) || codec == "ascii" || codec == "ai_cmd_response:json")
    {
        num_recv += num_print;
        num_print = 0;
        SPDLOG_INFO("Received {}", num_recv);
    }

    if (!loopback && headers.group_id == device_id)
    {
        return;
    }

    byte_buffer.emplace(data.begin(), data.end());
}

void TrackReader::StatusChanged(TrackReader::Status status)
{
    switch (status)
    {
    case Status::kOk:
    {
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            SPDLOG_INFO("Reader:Track {0} with alias: {1} is ready to read",
                        Stringify(GetFullTrackName()), track_alias.value());
        }
    }
    break;
    default:
        break;
    }
}

void TrackReader::AudioPlay()
{
    audio_playing =
        audio_playing || (!byte_buffer.empty() && byte_buffer.size() >= audio_min_depth);
}

void TrackReader::AudioPause()
{
    audio_playing = false;
}

bool TrackReader::AudioPlaying() const noexcept
{
    return audio_playing;
}

quicr::BytesSpan TrackReader::AudioFront() noexcept
{
    if (!audio_playing || byte_buffer.empty())
    {
        return quicr::BytesSpan();
    }

    return byte_buffer.front();
}

void TrackReader::AudioPop() noexcept
{
    byte_buffer.pop();
}

std::optional<std::vector<uint8_t>> TrackReader::AudioPopFront() noexcept
{
    if (!audio_playing || byte_buffer.empty())
    {
        return std::optional<std::vector<uint8_t>>();
    }

    std::optional<std::vector<uint8_t>> value = std::move(byte_buffer.front());
    byte_buffer.pop();
    return value;
}

size_t TrackReader::AudioNumAvailable() noexcept
{
    return byte_buffer.size();
}

void TrackReader::Stop()
{
    is_running = false;
}

bool TrackReader::TaskHasStopped() const noexcept
{
    return task_handle == nullptr;
}

void TrackReader::SubscribeTask(void* param)
{
    TrackReader* reader = static_cast<TrackReader*>(param);

    while (reader->is_running)
    {
        // Scope to reclaim variables
        {
            uint32_t next_print = 0;
            // TODO add in changing sub
            while (reader->GetStatus() != moq::TrackReader::Status::kOk)
            {
                if (esp_timer_get_time_ms() > next_print)
                {
                    NET_LOG_WARN("Subscriber on track %s waiting to for sub ok",
                                 reader->track_name.c_str());
                    next_print = esp_timer_get_time_ms() + 2000;
                }
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }

            NET_LOG_INFO("Subscribe to track %s", reader->track_name.c_str());
        }

        if (reader->codec == "pcm")
        {
            reader->TransmitAudio();
        }
        else if (reader->codec == "ascii")
        {
            reader->TransmitText();
        }
        else if (reader->codec == "ai_cmd_response:json")
        {
            reader->TransmitAiResponse();
        }
        else
        {
            NET_LOG_ERROR("Unknown codec, %s", reader->codec.c_str());
        }
    }

    NET_LOG_ERROR("Publish task for %s has exited", reader->track_name.c_str());
    vTaskDelete(nullptr);
}

void TrackReader::TransmitAudio()
{
    NET_LOG_INFO("Track reader - audio mode");
    while (GetStatus() == TrackReader::Status::kOk)
    {
        // TODO use notifies and then drain the entire moq objs
        vTaskDelay(2 / portTICK_PERIOD_MS);

        AudioPlay();
        if (AudioNumAvailable() == 0)
        {
            AudioPause();
            continue;
        }

        if (!AudioPlaying())
        {
            continue;
        }

        if (!xSemaphoreTake(audio_req_smpr, 0))
        {
            continue;
        }

        auto data = AudioPopFront();
        if (!data.has_value())
        {
            continue;
        }

        link_packet_t link_packet = {0};
        link_packet.type = (uint8_t)ui_net_link::Packet_Type::PttObject;
        link_packet.payload[0] = 0;
        link_packet.length = data->size() + 1;

        memcpy(link_packet.payload + 1, data->data(), data->size());

        serial.Write(link_packet);
    }
}

void TrackReader::TransmitText()
{
    NET_LOG_INFO("Track reader - text mode");

    while (GetStatus() == TrackReader::Status::kOk)
    {
        // TODO use notifies and then drain the entire moq objs
        vTaskDelay(2 / portTICK_PERIOD_MS);

        if (byte_buffer.empty())
        {
            continue;
        }

        std::optional<std::vector<uint8_t>> data = std::move(byte_buffer.front());
        byte_buffer.pop();

        link_packet_t link_packet = {0};
        link_packet.type = (uint8_t)ui_net_link::Packet_Type::TextMessage;
        link_packet.payload[0] = 0; // TODO channel id
        link_packet.length = data->size() + 1;

        memcpy(link_packet.payload + 1, data->data(), data->size());

        serial.Write(link_packet);
    }
}

void TrackReader::TransmitAiResponse()
{
    NET_LOG_INFO("Track reader - ai response mode");

    while (GetStatus() == TrackReader::Status::kOk)
    {
        vTaskDelay(2 / portTICK_PERIOD_MS);

        if (byte_buffer.empty())
        {
            continue;
        }

        std::optional<std::vector<uint8_t>> data = std::move(byte_buffer.front());
        byte_buffer.pop();

        link_packet_t link_packet = {0};
        link_packet.type = (uint8_t)ui_net_link::Packet_Type::AiResponse;
        link_packet.payload[0] = 0; // TODO channel id
        link_packet.length = data->size() + 1;

        // TODO split into smaller chunks
        NET_LOG_INFO("got a change channel len %d", data->size());

        memcpy(link_packet.payload + 1, data->data(), data->size());

        serial.Write(link_packet);
    }
}

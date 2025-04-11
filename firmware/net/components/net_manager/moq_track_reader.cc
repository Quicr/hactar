// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_reader.hh"
#include "utils.hh"
#include "moq_session.hh"
#include "chunk.hh"
#include "macros.hh"
#include "logger.hh"
#include "link_packet_t.hh"
#include "ui_net_link.hh"
#include "task_helpers.hh"

using namespace moq;

extern uint64_t device_id;
extern bool loopback;
extern SemaphoreHandle_t audio_req_smpr;

TrackReader::TrackReader(const quicr::FullTrackName& full_track_name,
    Serial& serial
):
    SubscribeTrackHandler(full_track_name,
        3,
        quicr::messages::GroupOrder::kAscending,
        quicr::messages::FilterType::kLatestObject),
    serial(serial),
    track_name(std::string(full_track_name.name_space.begin(), full_track_name.name_space.end()) + std::string(full_track_name.name.begin(), full_track_name.name.end())),
    audio_buffer(),
    audio_playing(false),
    audio_min_depth(50),
    audio_max_depth(std::numeric_limits<size_t>::max())
{
    task_helpers::Start_PSRAM_Task(SubscribeTask, this, track_name, task_handle, task_buffer, &task_stack, 8192, 10);
}


void TrackReader::ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data)
{
    ++num_print;

    if (num_print >= 20)
    {
        num_recv += num_print;
        num_print = 0;
        SPDLOG_INFO("Recieved {}", num_recv);
    }
    // SPDLOG_INFO("group id {} device id {}", headers.group_id, device_id);
    if (!loopback && headers.group_id == device_id)
    {
        return;
    }


    // SPDLOG_INFO("Object Received ({}): {}", headers.object_id, to_hex({ data.begin(), data.end() }));

    // Parse the data
    size_t offset = 1;
    switch ((MessageType)data[0])
    {
        case MessageType::Media:
        {
            // Since the content type for media is ALWAYS audio then
            // we assume it is audio and skip the first byte which is the
            // chunk type.
            // Let the task that sends out the audio data handle
            // the rest of the parsing
            // Skip the first byte
            audio_buffer.emplace(data.begin() + offset, data.end());
            break;
        }
        case MessageType::AIResponse:
        {
            uint32_t request_id = 0;
            std::memcpy(&request_id, data.data() + offset, sizeof(request_id));
            offset += sizeof(request_id);

            switch ((Content_Type)data[offset])
            {
                case Content_Type::Audio:
                {
                    offset += 1;
                    audio_buffer.emplace(data.begin() + offset, data.end());
                    break;
                }
                case Content_Type::Json:
                {
                    // TODO
                    break;
                }
                default:
                {
                    break;
                }

            }
            break;
        }
        default:
        {

            break;
        }
    }
}

void
TrackReader::StatusChanged(TrackReader::Status status)
{
    switch (status)
    {
        case Status::kOk:
        {
            if (auto track_alias = GetTrackAlias(); track_alias.has_value())
            {
                SPDLOG_INFO("Reader:Track {0} with alias: {1} is ready to read",
                    Stringify(GetFullTrackName()),
                    track_alias.value());
            }
        } break;
        default:
            break;
    }
}


void TrackReader::AudioPlay()
{
    audio_playing = audio_playing || (!audio_buffer.empty() && audio_buffer.size() >= audio_min_depth);
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
    if (!audio_playing || audio_buffer.empty())
    {
        return quicr::BytesSpan();
    }

    return audio_buffer.front();
}

void TrackReader::AudioPop() noexcept
{
    audio_buffer.pop();
}

std::optional<std::vector<uint8_t>> TrackReader::AudioPopFront() noexcept
{
    if (!audio_playing || audio_buffer.empty())
    {
        return std::optional<std::vector<uint8_t>>();
    }

    std::optional<std::vector<uint8_t>> value = std::move(audio_buffer.front());
    audio_buffer.pop();
    return value;
}


size_t TrackReader::AudioNumAvailable() noexcept
{
    return audio_buffer.size();
}

void TrackReader::SubscribeTask(void* param)
{
    TrackReader* reader = static_cast<TrackReader*>(param);

    while (true)
    {
        // Scope to reclaim variables
        {
            uint32_t next_print = 0;
            // TODO add in changing sub
            while (reader->GetStatus() != moq::TrackReader::Status::kOk)
            {
                if (esp_timer_get_time_ms() > next_print)
                {
                    NET_LOG_WARN("Subscriber on track %s waiting to for sub ok", reader->track_name.c_str());
                    next_print = esp_timer_get_time_ms() + 2000;
                }
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }

            NET_LOG_INFO("Subscribe to track %s", reader->track_name.c_str());

            // TODO changing sub
            while (reader->GetStatus() == TrackReader::Status::kOk)
            {
                // TODO use notifies and then drain the entire moq objs
                vTaskDelay(2 / portTICK_PERIOD_MS);

                // TODO move into one function
                // TODO all of these continues could probably be condensed
                reader->AudioPlay();
                if (reader->AudioNumAvailable() == 0)
                {
                    reader->AudioPause();
                    continue;
                }

                if (!reader->AudioPlaying())
                {
                    continue;
                }

                if (!xSemaphoreTake(audio_req_smpr, 0))
                {
                    continue;
                }

                auto data = reader->AudioPopFront();
                if (!data.has_value())
                {
                    continue;
                }

                uint32_t offset = 0;

                if (data->at(offset))
                {
                    // last chunk
                }
                offset += 1;

                link_packet_t link_packet;
                // TODO need to get the type from the link_obj type
                link_packet.type = (uint8_t)ui_net_link::Packet_Type::PttObject;

                // Get the length of the audio packet
                uint32_t audio_length = 0;
                memcpy(&audio_length, data->data() + offset, sizeof(audio_length));
                offset += sizeof(audio_length);


                // Channel id
                // TODO get channel id, for now use 0
                // I think the channel id can just be a hash if need
                // be, but that seems odd to use.
                link_packet.payload[0] = 0;
                link_packet.length = audio_length + 1;

                memcpy(link_packet.payload + 1, data->data() + offset, audio_length);

                // NET_LOG_INFO("packet length %d", link_packet.length);

                reader->serial.Write(link_packet);
            }
        }
    }

    NET_LOG_ERROR("Publish task for %s has exited", reader->track_name.c_str());
    vTaskDelete(nullptr);
}

// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#include "moq_track_reader.hh"
#include "chunk.hh"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "link_packet_t.hh"
#include "logger.hh"
#include "macros.hh"
#include "moq_session.hh"
#include "net.hh"
#include "net_mgmt_link.h"
#include "portmacro.h"
#include "task_helpers.hh"
#include "ui_net_link.hh"
#include "utils.hh"
#include <cstdint>

using namespace moq;

TrackReader::TrackReader(const quicr::FullTrackName& full_track_name,
                         Serial& serial,
                         const std::string& codec,
                         const Runtime& runtime,
                         const Diagnostics& diagnostics) :
    SubscribeTrackHandler(full_track_name, 3, quicr::messages::GroupOrder::kAscending),
    serial(serial),
    codec(codec),
    runtime(runtime),
    diagnostics(diagnostics),
    track_name(std::string(full_track_name.name_space.begin(), full_track_name.name_space.end())
               + std::string(full_track_name.name.begin(), full_track_name.name.end())),
    moq_packets(Moq_Objects_Packet_Num),
    task_mutex(),
    audio_playing(false),
    audio_min_depth(Jitter_Buff_Size),
    audio_max_depth(std::numeric_limits<size_t>::max()),
    task_handle(nullptr),
    task_buffer{},
    task_stack(nullptr),
    ai_request_id(0),
    num_print(0),
    num_recv(0),
    is_running(false)
{
}

TrackReader::~TrackReader()
{
    if (!is_running)
    {
        return;
    }

    Stop();
}

void TrackReader::Start()
{
    if (task_handle)
    {
        NET_LOG_INFO("task handle already exists %s", track_name.c_str());
        return;
    }

    NET_LOG_INFO("Start subscriber task for trackname %s", track_name.c_str());

    task_helpers::Start_PSRAM_Task(SubscribeTask, this, track_name, task_handle, task_buffer,
                                   &task_stack, 8192, 10);
}

void TrackReader::Stop()
{
    NET_LOG_ERROR("Stopping reader");
    is_running = false;
    // Lock and wait until the task is done or hasn't started and it will just continue
    std::lock_guard<std::mutex> _(task_mutex);

    if (task_stack)
    {
        heap_caps_free(task_stack);
        task_stack = nullptr;
    }

    NET_LOG_ERROR("reader has stopped");
}

void TrackReader::ObjectReceived(const quicr::ObjectHeaders& headers,
                                 quicr::BytesSpan data,
                                 std::optional<quicr::messages::StreamHeaderProperties> stream_mode)
{
    ++num_print;

    if ((codec == "pcm" && num_print >= 20) || codec == "ascii" || codec == "ai_cmd_response:json")
    {
        num_recv += num_print;
        num_print = 0;
        NET_LOG_DEBUG("%s Received %llu", Stringify(GetFullTrackName()).c_str(), num_recv);
    }

    if (diagnostics.loopback != NetLoopbackMode::Moq && headers.group_id == runtime.device_id)
    {
        return;
    }

    if (data.size() > Moq_Objects_Size)
    {
        NET_LOG_ERROR("Received object was larger than can be handled by link packet");
        return;
    }

    moq_packet_t* moq_packet = moq_packets.BorrowWrite();

    memcpy(moq_packet->data, data.data(), data.size());
    moq_packet->len = data.size();

    moq_packets.AdvWrite();

    xTaskNotifyGive(task_handle);
}

void TrackReader::StatusChanged(TrackReader::Status status)
{
    switch (status)
    {
    case Status::kOk:
    {
        if (auto track_alias = GetTrackAlias(); track_alias.has_value())
        {
            NET_LOG_INFO("Reader:Track %s with alias: %u is ready to read",
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
        audio_playing || (moq_packets.Unread() > 0 && moq_packets.Unread() >= audio_min_depth);
}

void TrackReader::AudioPause()
{
    audio_playing = false;
}

bool TrackReader::AudioPlaying() const noexcept
{
    return audio_playing;
}

const std::string& TrackReader::GetTrackName() const noexcept
{
    return track_name;
}

void TrackReader::SubscribeTask(void* param)
{
    TrackReader* reader = static_cast<TrackReader*>(param);
    std::lock_guard<std::mutex> _(reader->task_mutex);
    reader->is_running = true;

    while (reader->is_running)
    {
        // Scope to reclaim variables
        {
            uint32_t next_print = 0;
            // TODO add in changing sub
            while (reader->GetStatus() != moq::TrackReader::Status::kOk && reader->is_running)
            {
                if (esp_timer_get_time_ms() > next_print)
                {
                    NET_LOG_WARN("Subscriber on track %s waiting to for sub ok, status %d",
                                 reader->track_name.c_str(), (int)reader->GetStatus());
                    next_print = esp_timer_get_time_ms() + 5000;
                }
                vTaskDelay(300 / portTICK_PERIOD_MS);
            }

            NET_LOG_INFO("Subscribe to track %s", reader->track_name.c_str());
        }

        if (reader->codec == "pcm")
        {
            reader->TransmitAudio();
        }
        else if (reader->codec == "ascii" || reader->codec == "ai_cmd_response:json")
        {
            reader->TransmitText();
        }
        else
        {
            NET_LOG_ERROR("Unknown codec, %s", reader->codec.c_str());
        }
    }

    NET_LOG_ERROR("Sub task for %s has exited", reader->track_name.c_str());

    reader->task_mutex.unlock();
    vTaskDelete(nullptr);
}

void TrackReader::TransmitAudio()
{
    NET_LOG_INFO("Track reader %s - audio mode", track_name.c_str());
    while (GetStatus() == TrackReader::Status::kOk && is_running)
    {
        if (!ulTaskNotifyTake(pdTRUE, portMAX_DELAY) || moq_packets.Unread() == 0)
        {
            continue;
        }

        // Lock the shared audio transmit sempr
        while (!xSemaphoreTake(runtime.audio_lock_sempr, portMAX_DELAY))
        {
            continue;
        }

        // Sync the audio interrupt, by eating the stale request
        while (!xSemaphoreTake(runtime.audio_req_smpr, portMAX_DELAY))
        {
            continue;
        }

        uint16_t miss = 0;
        while (true)
        {
            if (miss >= 5)
            {
                AudioPause();
                break;
            }

            AudioPlay();
            if (moq_packets.Unread() == 0 || !AudioPlaying())
            {
                ++miss;
                continue;
            }

            while (!xSemaphoreTake(runtime.audio_req_smpr, portMAX_DELAY))
            {
                continue;
            }

            WriteToSerial(moq_packets.Read());
        }

        xSemaphoreGive(runtime.audio_lock_sempr);
    }
}

void TrackReader::TransmitText()
{
    NET_LOG_INFO("Track reader %s - text mode", track_name.c_str());

    while (GetStatus() == TrackReader::Status::kOk && is_running)
    {
        if (!ulTaskNotifyTake(pdTRUE, portMAX_DELAY) || moq_packets.Unread() == 0)
        {
            continue;
        }

        // Lock the shared audio transmit sempr
        while (!xSemaphoreTake(runtime.audio_lock_sempr, portMAX_DELAY))
        {
            continue;
        }

        // Drain all available packets
        while (moq_packets.Unread() > 0)
        {
            WriteToSerial(moq_packets.Read());
        }

        xSemaphoreGive(runtime.audio_lock_sempr);
    }
}

void TrackReader::WriteToSerial(const moq_packet_t& moq_packet)
{
    serial.Write(link_packet_t::Sync_Word, sizeof(link_packet_t::Sync_Word));
    const uint16_t type = static_cast<uint16_t>(ui_net_link::NetToUi::AudioFrame);
    serial.Write((uint8_t*)&type, sizeof(type));
    const uint32_t ext_len = moq_packet.len + 1;
    serial.Write((uint8_t*)&ext_len, sizeof(ext_len));
    serial.Write(0); // todo Channel id
    serial.Write(moq_packet.data, moq_packet.len);
}

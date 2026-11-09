// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_TRACK_READER__
#define __MOQ_TRACK_READER__

#include "freertos/idf_additions.h"
#include "link_packet_t.hh"
#include "net.hh"
#include "ring_buffer.hh"
#include "serial.hh"
#include <quicr/client.h>
#include <cstdint>
#include <cwchar>
#include <deque>

namespace moq
{

class TrackReader : public quicr::SubscribeTrackHandler
{
private:
    static constexpr size_t Jitter_Buff_Size = 5;
    static constexpr size_t Moq_Objects_Packet_Num = Jitter_Buff_Size * 2;
    static constexpr size_t Moq_Objects_Size = link_packet_t::Packet_Size - 1;

public:
    TrackReader(const quicr::FullTrackName& full_track_name,
                Serial& serial,
                const std::string& codec,
                const Runtime& runtime,
                const Diagnostics& diagnostics);

    virtual ~TrackReader();

    void Start();

    void Stop();

    //
    // overrides from SubscribeTrackHandler
    //
    void ObjectReceived(
        const quicr::ObjectHeaders& headers,
        quicr::BytesSpan data,
        std::optional<quicr::messages::StreamHeaderProperties> stream_mode = std::nullopt) override;

    void StatusChanged(Status status) override;

    void AudioPlay();
    void AudioPause();

    bool AudioPlaying() const noexcept;

    const std::string& GetTrackName() const noexcept;

private:
    struct moq_packet_t
    {
        uint8_t data[Moq_Objects_Size];
        size_t len;
    };

    static void SubscribeTask(void* param);

    void TransmitAudio();
    void TransmitText();

    void WriteToSerial(const moq_packet_t& moq_packet);

    Serial& serial;
    const std::string codec;
    const Runtime& runtime;
    const Diagnostics& diagnostics;

    std::string track_name;

    RingBuffer<moq_packet_t> moq_packets;

    std::mutex task_mutex;

    bool audio_playing;
    size_t audio_min_depth;
    size_t audio_max_depth;

    TaskHandle_t task_handle;
    StaticTask_t task_buffer;
    StackType_t* task_stack;

    uint32_t ai_request_id;
    uint64_t num_print;
    uint64_t num_recv;

    bool is_running;
    std::mutex packet_mux;
};

} // namespace moq

#endif

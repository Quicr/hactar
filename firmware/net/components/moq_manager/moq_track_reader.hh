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
    static constexpr size_t Moq_Objs_Packet_Num = Jitter_Buff_Size * 2;
    static constexpr size_t Moq_Objs_Size = link_packet_t::Packet_Size * Moq_Objs_Packet_Num;

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

    // quicr::BytesSpan AudioFront() noexcept;
    // void AudioPop() noexcept;
    // std::optional<std::vector<uint8_t>> AudioPopFront() noexcept;
    // size_t AudioNumAvailable() noexcept;

    const std::string& GetTrackName() const noexcept;

private:
    static void SubscribeTask(void* param);

    void TransmitAudio();
    void TransmitText();

    void WriteToSerial(const uint8_t* data, const size_t len);

    Serial& serial;
    const std::string codec;
    const Runtime& runtime;
    const Diagnostics& diagnostics;

    std::string track_name;
    // TODO rename to link_packet_buffer

    // std::queue<std::vector<uint8_t>> byte_buffer;

    // TODO num
    // RingBuffer<uint8_t> moq_objs;
    uint8_t moq_objs[Moq_Objs_Size];
    size_t write_idx;
    size_t read_idx;
    RingBuffer<size_t> moq_objs_len;

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

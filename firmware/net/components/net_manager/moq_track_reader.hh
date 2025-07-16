// SPDX-FileCopyrightText: Copyright (c) 2024 Cisco Systems
// SPDX-License-Identifier: BSD-2-Clause

#ifndef __MOQ_TRACK_READER__
#define __MOQ_TRACK_READER__

#include "serial.hh"
#include <quicr/client.h>
#include <cwchar>
#include <deque>

namespace moq
{

class TrackReader : public quicr::SubscribeTrackHandler
{
public:
    TrackReader(const quicr::FullTrackName& full_track_name,
                Serial& serial,
                const std::string& codec);

    virtual ~TrackReader();

    void Start();

    void Stop();

    //
    // overrides from SubscribeTrackHandler
    //
    void ObjectReceived(const quicr::ObjectHeaders& headers, quicr::BytesSpan data) override;

    void StatusChanged(Status status) override;

    void AudioPlay();
    void AudioPause();

    bool AudioPlaying() const noexcept;

    quicr::BytesSpan AudioFront() noexcept;
    void AudioPop() noexcept;
    std::optional<std::vector<uint8_t>> AudioPopFront() noexcept;
    size_t AudioNumAvailable() noexcept;

    const std::string& GetTrackName() const noexcept;

private:
    static void SubscribeTask(void* param);

    void TransmitAudio();
    void TransmitText();
    void TransmitAiResponse();
    // Audio variables
    // TODO move into an audio object?
    Serial& serial;
    const std::string codec;
    std::string track_name;
    // TODO rename to link_packet_buffer
    std::queue<std::vector<uint8_t>> byte_buffer;

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
};

} // namespace moq

#endif

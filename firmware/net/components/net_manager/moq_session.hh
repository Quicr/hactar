#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include "constants.hh"
#include "moq_track_reader.hh"
#include "moq_track_writer.hh"
#include "uart.hh"
#include <nlohmann/json.hpp>
#include <quicr/client.h>
#include <quicr/detail/defer.h>
#include <map>

namespace moq
{

using json = nlohmann::json;

/**
 * MoQSession identifies a client session with the MOQ Peer
 */

class Session : public quicr::Client
{
public:
    using quicr::Client::Client;

    Session(const quicr::ClientConfig& cfg,
            std::vector<std::shared_ptr<TrackReader>>& readers,
            std::vector<std::shared_ptr<TrackWriter>>& writers);

    virtual ~Session() = default;

    void StopTracks() noexcept;

    void StatusChanged(Status status) override;

    // public API - send subscribe, setup queue for incoming objects
    void Read();

    void Write();

    std::shared_ptr<TrackReader> Reader(const size_t id) noexcept;
    std::shared_ptr<TrackWriter> Writer(const size_t id) noexcept;

    void StopAudioReader();
    void StopTextReader();
    void StopAudioWriter();

    // TODO delete after moq session can be reused
    std::unique_lock<std::mutex> GetReaderLock();
    std::unique_lock<std::mutex> GetWriterLock();

private:
    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = delete;

    static void PublishTrackTask(void* params);
    static void SubscribeTrackTask(void* params);

    void StartTasks() noexcept;

    std::vector<std::shared_ptr<TrackReader>>& readers;
    TaskHandle_t readers_task_handle;
    StaticTask_t readers_task_buffer;
    StackType_t* readers_task_stack;

    std::vector<std::shared_ptr<TrackWriter>>& writers;
    TaskHandle_t writers_task_handle;
    StaticTask_t writers_task_buffer;
    StackType_t* writers_task_stack;

    std::mutex readers_mux;
    std::mutex writers_mux;
};

} // namespace moq
#endif

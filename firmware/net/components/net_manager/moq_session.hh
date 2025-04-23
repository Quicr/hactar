#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include "constants.hh"
#include "moq_track_reader.hh"
#include "moq_track_writer.hh"
#include "serial.hh"
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

    Session(const quicr::ClientConfig& cfg);

    virtual ~Session() = default;

    void StatusChanged(Status status) override;

    // public API - send subscribe, setup queue for incoming objects
    void StartReadTrack(const json& subscription, Serial& serial);
    void Read();
    void StopReadTrack(const std::string& track_name);

    void StartWriteTrack(const json& publication);
    void Write();
    void StopWriteTrack(const std::string& track_name);

    std::shared_ptr<TrackReader> Reader(const size_t id) noexcept;
    std::shared_ptr<TrackWriter> Writer(const size_t id) noexcept;

private:
    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = delete;

    static void PublishTrackTask(void* params);
    static void SubscribeTrackTask(void* params);

    void StartTasks() noexcept;

    std::vector<std::shared_ptr<TrackReader>> readers;
    std::mutex readers_mux;
    TaskHandle_t readers_task_handle;
    StaticTask_t readers_task_buffer;
    StackType_t* readers_task_stack;

    std::vector<std::shared_ptr<TrackWriter>> writers;
    std::mutex writers_mux;
    TaskHandle_t writers_task_handle;
    StaticTask_t writers_task_buffer;
    StackType_t* writers_task_stack;
};

} // namespace moq
#endif

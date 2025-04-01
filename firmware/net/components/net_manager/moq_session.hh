#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include "moq_track_reader.hh"
#include "moq_audio_track_reader.hh"
#include "moq_track_writer.hh"
#include "constants.hh"

#include <quicr/client.h>
#include <quicr/detail/defer.h>

#include <map>


namespace moq
{
/**
* MoQSession identifies a client session with the MOQ Peer
*/

union Chunk
{
    uint8_t type;
    uint8_t is_last;
    uint32_t length;
    uint8_t* data;
};

class Session: public quicr::Client
{
public:
    using quicr::Client::Client;

    virtual ~Session() = default;

    void StatusChanged(Status status) override;

    // public API - send subscribe, setup queue for incoming objects
    void StartReadTrack(const std::string& track_name);
    void StartAudioReadTrack(const std::string& track_name, const size_t min_depth, const size_t max_depth);
    void Read();
    void StopReadTrack(const std::string& track_name);

    void StartWriteTrack(const std::string& track_name);
    void Write();
    void StopWriteTrack(const std::string& track_name);

private:
    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = delete;

    struct TrackState
    {
        std::shared_ptr<TrackReader> reader;
    };

private:
    std::map<quicr::TrackHash, std::shared_ptr<TrackReader>> readers;
    std::map<quicr::TrackHash, std::shared_ptr<TrackWriter>> writers;
};

}
#endif

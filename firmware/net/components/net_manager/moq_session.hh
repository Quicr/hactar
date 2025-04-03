#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include "moq_track_reader.hh"
#include "moq_track_writer.hh"
#include "constants.hh"

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

enum class MessageType
{
    Media = 1,
    AIRequest,
    AIResponse
};

enum class Content_Type
{
    Audio = 0,
    Json,
};

struct Chunk
{
    const MessageType type = MessageType::Media;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};

struct AIRequestChunk
{
    const MessageType type = MessageType::AIRequest;
    std::uint32_t request_id;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};

struct AIResponseChunk
{
    const MessageType type = MessageType::AIResponse;
    std::uint32_t request_id;
    Content_Type content_type;
    bool last_chunk;
    std::uint32_t chunk_length;
    quicr::Bytes chunk_data;
};

class Session: public quicr::Client
{
public:
    using quicr::Client::Client;

    virtual ~Session() = default;

    void StatusChanged(Status status) override;

    // public API - send subscribe, setup queue for incoming objects
    void StartReadTrack(const std::string& track_name);
    void Read();
    void StopReadTrack(const std::string& track_name);

    void StartWriteTrack(const json track_json);
    void Write();
    void StopWriteTrack(const std::string& track_name);

    std::vector<std::shared_ptr<TrackReader>>& Readers() noexcept;
    std::vector<std::shared_ptr<TrackReader>>& Writers() noexcept;

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
    std::vector<std::shared_ptr<TrackReader>> readers;
    std::vector<std::shared_ptr<TrackReader>> writers;
};

}
#endif

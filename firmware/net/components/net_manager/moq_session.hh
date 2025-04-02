#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include "moq_track_reader.hh"
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

    std::shared_ptr<TrackWriter> writer;
    std::shared_ptr<TrackWriter> ai_writer;
};

}
#endif

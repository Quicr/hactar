#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include <quicr/client.h>
#include <map>

#include "moq_track_reader.hh"

namespace moqt {
/**
* MoQSession identifies a client session with the MOQ Peer
*/
class Session : public quicr::Client {
public:
    static std::shared_ptr <Session> Create(const quicr::ClientConfig &cfg) {
        return std::shared_ptr<Session>(new Session(cfg));
    }

    Session(const quicr::ClientConfig &cfg)
            : quicr::Client(cfg) {}

    void StatusChanged(Status status) override;

    // public API - send subscribe, setup queue for incoming objects
    void start_read_track(const std::string& track_name);
    void read();
    void stop_read_track(const std::string& track_name);

    void start_write_track(const std::string& track_name);
    void write();
    void stop_write_track(const std::string& track_name);

private:

    Session() = delete;
    Session(const Session &) = delete;
    Session(Session &&) noexcept = delete;
    Session &operator=(const Session &) = delete;
    Session &operator=(Session &&) noexcept = delete;

    struct TrackState {
        std::shared_ptr<TrackReader> reader;
    };

    std::map<std::string, std::shared_ptr<TrackReader>> readers;

};

}
#endif
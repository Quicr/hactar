#ifndef __MOQT_SESSION__
#define __MOQT_SESSION__

#include <quicr/client.h>

/**
 * MoQSession identifies a client session with the MOQ Peer
 */
class Session : public quicr::Client
{
public:
    static std::shared_ptr<Session> Create(const quicr::ClientConfig& cfg, bool stop_threads)
    {
        return std::shared_ptr<Session>(new Session(cfg, stop_threads));
    }

    Session(const quicr::ClientConfig& cfg, bool stop_threads)
            : quicr::Client(cfg)
    {}

    virtual ~Session() = default;
    void StatusChanged(Status status) override;

private:
    Session() = delete;
    Session(const Session&) = delete;
    Session(Session&&) noexcept = delete;
    Session& operator=(const Session&) = delete;
    Session& operator=(Session&&) noexcept = delete;


    // TODO: use this
    bool stop_threads_;
};

#endif
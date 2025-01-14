#include "moq_session.hh"

#include "logger.hh"

using namespace moq;

void Session::StatusChanged(Status status)
{
    switch (status) {
        case Status::kReady:
            Logger::Log(Logger::Level::Info, "MOQ Connection ready");
            break;
        case Status::kConnecting:
            break;
        case Status::kPendingSeverSetup:
            Logger::Log(Logger::Level::Info, "MOQ Connection connected and now pending server setup");
            break;
        default:
            Logger::Log(Logger::Level::Error, "MOQ Connection failed: ", static_cast<int>(status));
            break;
    }
}

#include <moq_session.hh>

using namespace moqt;


void Session::StatusChanged(int status) {
    switch (status) {
        case Status::kReady:
            SPDLOG_INFO("Connection ready");
            break;
        case Status::kConnecting:
            break;
        case Status::kPendingSeverSetup:
            SPDLOG_INFO("Connection connected and now pending server setup");
            break;
        default:
            SPDLOG_INFO("Connection failed {0}", static_cast<int>(status));
            stop_threads_ = true;
            break;
    }
}
#include "moq_session.hh"

using namespace moqt;

void Session::StatusChanged(Status status) {
    switch (status) {
        case Status::kReady:
            break;
            case Status::kConnecting:
                break;
            case Status::kPendingSeverSetup:
                break;
                default:
                    break;
}
}


#pragma once

#include <memory>
#include <string>
#include <vector>

class Runtime
{

    struct MoqRuntime
    {
        std::shared_ptr<moq::Session> session;
        std::vector<std::shared_ptr<moq::TrackReader>> readers;
        std::vector<std::shared_ptr<moq::TrackWriter>> writers;
        SemaphoreHandle_t audio_req_smpr;
    };

    struct Runtime
    {
        uint64_t device_id;
        Peripherals periphals;

        MoqRuntime moq;
    };
};

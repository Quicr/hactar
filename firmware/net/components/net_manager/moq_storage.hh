#pragma once

#include "storage.hh"
#include <mutex>

class MoqStorage
{
public:
    static constexpr const char* Default_Moq_Server_Uri =
        "moq://relay.us-west-2.quicr.ctgpoc.com:33435";

    MoqStorage(Storage& storage);
    const std::string& LoadMoqServerUrl();
    void SaveMoqServerUrl(const std::string& str);

private:
    Storage& storage;
    std::string moq_server_url;
    std::mutex mux;
};
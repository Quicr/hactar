#include "moq_storage.hh"
#include "logger.hh"

MoqStorage::MoqStorage(Storage& storage) :
    storage(storage),
    moq_server_url(),
    mux()
{
}

const std::string& MoqStorage::LoadMoqServerUrl()
{
    // Check if already loaded moq_server_url
    if (!moq_server_url.empty())
    {
        return moq_server_url;
    }

    std::lock_guard<std::mutex> _(mux);
    moq_server_url = storage.LoadStr("moq_storage", "moq_server_url");

    NET_LOG_INFO("Loading moq server url len %d - %s", moq_server_url.length(),
                 moq_server_url.c_str());

    if (moq_server_url == "")
    {
        NET_LOG_WARN("Failed to load moq server url, using default %s", Default_Moq_Server_Uri);

        moq_server_url = Default_Moq_Server_Uri;
    }

    return moq_server_url;
}

void MoqStorage::SaveMoqServerUrl(const std::string& str)
{
    std::lock_guard<std::mutex> _(mux);
    if (str.length() == 0)
    {
        storage.ClearKey("moq_storage", "moq_server_url");
    }
    else
    {
        storage.SaveStr("moq_storage", "moq_server_url", str);
    }

    moq_server_url = str;
}
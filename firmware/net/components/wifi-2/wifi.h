#pragma once

#include <cstring>
#include <mutex>

#include "esp_wifi.h"
#include "esp_event.h"

namespace net_wifi
{

class Wifi
{
public:
    enum class State
    {
        NotInitialized=0,
        Initialized,
        ReadyToConnect,
        Connecting,
        WaitingForIP,
        Connected,
        Disconnected,
        Error
    };

    void SetCredentials(const char *ssid, const char *password);
    esp_err_t init();
    esp_err_t connect();
    constexpr static const State &get_state() { return state; }

private:
    
    static esp_err_t initialize();
    static wifi_init_config_t wifi_init_cfg;
    static wifi_config_t wifi_cfg;

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);
    static void ip_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

    static State state;
    static std::mutex state_mutex;
};

} // namaspace
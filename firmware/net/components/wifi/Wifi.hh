#pragma once

#include <cstring>
#include <mutex>

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "Vector.hh"
#include "String.hh"

#define MAX_AP 10

namespace hactar_utils
{

class Wifi
{
public:
    enum class State
    {
        NotInitialized = 0,
        Initialized,
        ReadyToConnect,
        Connecting,
        WaitingForIP,
        Connected,
        Disconnected,
        Error
    };

    Wifi();
    ~Wifi() = default;
    
    Wifi(Wifi& other) = delete;
    void operator=(const Wifi& other) = delete;
    static Wifi* GetInstance();

    esp_err_t Connect();
    esp_err_t Connect(const char* ssid, const char* password);
    esp_err_t Initialize();
    esp_err_t Deinitialize();
    esp_err_t Disconnect();
    esp_err_t ScanNetworks(Vector<String>* ssids);
    void SetCredentials(const char* ssid, const char* password);

    State GetState() const;
    bool IsConnected() const;

    

private:


    // Static functions
    // ESP events need to be static
    static void EventHandler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

    // Static variables
    // For singleton
    static Wifi* instance;
    static std::mutex state_mutex;

    inline void WifiEvents(int32_t event_id, void* event_data);
    inline void IpEvents(int32_t event_id);

    wifi_init_config_t wifi_init_cfg;
    wifi_config_t wifi_cfg;
    State state;
};

} // namaspace
#pragma once

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include <cstring>
#include <mutex>
#include <string>
#include <vector>

// TODO scrolling
#define MAX_AP 10

#define MAX_ATTEMPTS 5

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
        InvalidCredentials,
        Disconnected,
        Error,
        UnableToConnect,
        Scan,
        Scanning
    };

    Wifi(const int64_t scan_timeout_ms = 15'000);
    Wifi(Wifi& other) = delete;
    ~Wifi();
    void operator=(const Wifi& other) = delete;

    void Begin();

    const std::vector<std::string>& GetNetworksInRange() const;
    void Connect(const std::string& ssid, const std::string& pwd);
    esp_err_t Disconnect();

    State GetState() const;
    bool IsConnected() const;
    bool IsInitialized() const;

    struct ap_cred_t
    {
        std::string ssid;
        std::string pwd;
        uint32_t attempts;
    };

private:
    esp_err_t Initialize();
    // TODO test deinit and init for deep sleep.
    esp_err_t Deinitialize();
    esp_err_t ScanNetworks();

    void Connect();

    // Static functions
    // ESP events need to be static
    static void WifiTask(void* arg);
    static void EventHandler(void* arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data);


    // Private functions
    inline void WifiEvents(int32_t event_id, void* event_data);
    inline void IpEvents(int32_t event_id);

    // Private variables
    int64_t scan_timeout_ms;

    std::vector<ap_cred_t> credentials;
    std::vector<ap_cred_t> ap_in_range;
    std::vector<std::string> scanned_aps;

    int32_t ap_idx;
    int64_t last_ssid_scan;
    int64_t retry_timeout;

    wifi_init_config_t wifi_init_cfg;
    wifi_config_t wifi_cfg;
    State state;
    State prev_state;

    bool is_initialized;

    TaskHandle_t connect_task;
    std::mutex state_mux;
};

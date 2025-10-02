#pragma once

#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "storage.hh"
#include "stored_value.hh"
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
    static constexpr size_t Max_SSID_Name_Len = 32;
    static constexpr size_t Max_SSID_Password_Len = 64;
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

    Wifi(Storage& storage, const int64_t scan_timeout_ms = 15'000);
    Wifi(Wifi& other) = delete;
    ~Wifi();
    void operator=(const Wifi& other) = delete;

    void Begin();

    const std::vector<std::string>& GetNetworksInRange() const;
    void Connect(const std::string& ssid, const std::string& pwd);
    void SaveSSID(const std::string ssid, const std::string pwd);
    void ClearSavedSSIDs();
    void SaveSSIDName(const std::string& ssid);
    void SaveSSIDPwd(const std::string& pwd);
    std::string LoadSSIDName(const uint32_t ssid_num);
    std::string LoadSSIDNames();
    std::string LoadSSIDPassword(const uint32_t ssid_num);
    std::string LoadSSIDPasswords();
    esp_err_t Disconnect();

    State GetState() const;
    bool IsConnected() const;
    bool IsInitialized() const;

    struct ap_cred_t
    {
        // ap cred names and pwd are max 32 bytes, but need a extra char for null characters
        char name[Max_SSID_Name_Len];
        uint32_t name_len;
        char pwd[Max_SSID_Password_Len];
        uint32_t pwd_len;
        uint32_t attempts;
    };

    const std::vector<ap_cred_t>& GetCredentials();

private:
    esp_err_t Initialize();
    // TODO test deinit and init for deep sleep.
    esp_err_t Deinitialize();
    esp_err_t ScanNetworks();

    void Connect();

    // Static functions
    // ESP events need to be static
    static void WifiTask(void* arg);
    static void
    EventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

    // Private functions
    inline void WifiEvents(int32_t event_id, void* event_data);
    inline void IpEvents(int32_t event_id);

    // Private variables
    Storage& storage;
    int64_t scan_timeout_ms;

    StoredValue<std::vector<ap_cred_t>> stored_creds;
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

    size_t saved_ssids;

    TaskHandle_t connect_task;
    std::mutex state_mux;

    std::string tmp_ssid;
    std::string tmp_pwd;
};

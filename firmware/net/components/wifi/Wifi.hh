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

#define MAX_ATTEMPTS 1

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
        Error,
        InvalidCredentials
    };

    Wifi(Wifi& other) = delete;
    void operator=(const Wifi& other) = delete;
    static Wifi* GetInstance();

    void Connect(const char* ssid, const char* password);
    void Connect(
        const char* ssid,
        const size_t ssid_len,
        const char* password,
        const size_t password_len);
    esp_err_t Disconnect();
    esp_err_t ScanNetworks(std::vector<std::string>* ssids);

    State GetState() const;
    bool IsConnected() const;

protected:
    Wifi();
    ~Wifi();


private:
    typedef struct
    {
        char* ssid;
        size_t ssid_len;
        char* password;
        const size_t password_len;
    } wifi_creds;

    esp_err_t Initialize();
    // TODO test deinit and init for deep sleep.
    esp_err_t Deinitialize();

    // Static functions
    // ESP events need to be static
    static void ConnectTask(void* params);
    static void EventHandler(void* arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void* event_data);

    // Static variables
    // For singleton
    static Wifi* instance;

    // Private functions
    inline void WifiEvents(int32_t event_id, void* event_data);
    inline void IpEvents(int32_t event_id);

    // Private variables
    wifi_init_config_t wifi_init_cfg;
    wifi_config_t wifi_cfg;
    State state;
    TaskHandle_t connect_task;
    SemaphoreHandle_t connect_semaphore;

    uint8_t failed_attempts = 0;
};

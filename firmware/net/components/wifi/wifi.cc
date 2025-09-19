#include "wifi.hh"
#include "../../core/inc/macros.hh"
#include "esp_log.h"
#include "logger.hh"
#include <algorithm>
#include <bit>
#include <iostream>
#include <mutex>

Wifi::Wifi(Storage& storage, const int64_t scan_timeout_ms) :
    storage(storage),
    scan_timeout_ms(scan_timeout_ms),
    credentials(),
    ap_in_range(),
    scanned_aps(),
    ap_idx(0),
    last_ssid_scan(-scan_timeout_ms), // This will let us immediately scan once we are initialized
                                      // without adding another var
    wifi_init_cfg(WIFI_INIT_CONFIG_DEFAULT()),
    wifi_cfg({}),
    state(State::NotInitialized),
    is_initialized(false),
    saved_ssids(0),
    connect_task(),
    state_mux()
{
}

Wifi::~Wifi()
{
    NET_LOG_INFO("Delete wifi");
    vTaskDelete(connect_task);
}

void Wifi::Begin()
{
    // Get the number of saved ssids
    saved_ssids = storage.Loadu32("wifi", "saved_ssids");

    char buff[32];
    ssize_t len = 0;
    std::string ssid;
    std::string pwd;

    // Load all ssids
    for (size_t i = 0; i < saved_ssids; ++i)
    {
        len = storage.Load("wifi", "ssid_name" + std::to_string(i + 1), buff, sizeof(buff));
        ssid.assign(buff, len);

        len = storage.Load("wifi", "ssid_pwd" + std::to_string(i + 1), buff, sizeof(buff));
        pwd.assign(buff, len);

        Connect(ssid, pwd);
    }

    xTaskCreate(WifiTask, "wifi_connect_task", 4096, (void*)this, 12, &connect_task);
}

const std::vector<std::string>& Wifi::GetNetworksInRange() const
{
    return scanned_aps;
}

void Wifi::Connect(const std::string& ssid, const std::string& pwd)
{
    if (ssid.empty() || pwd.empty())
    {
        return;
    }

    credentials.push_back({ssid, pwd, 0});
}

void Wifi::SaveSSID(const std::string ssid, const std::string pwd)
{
    esp_err_t err = storage.Save("wifi", "ssid_name" + std::to_string(saved_ssids + 1), ssid.data(),
                                 ssid.length());

    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to save ssid %s", ssid.c_str());
        return;
    }

    err = storage.Save("wifi", "ssid_pwd" + std::to_string(saved_ssids + 1), pwd.data(),
                       pwd.length());

    if (err != ESP_OK)
    {
        NET_LOG_ERROR("Failed to save ssid pwd %s", pwd.c_str());
        return;
    }

    saved_ssids += 1;
    Connect(ssid, pwd);
}

void Wifi::ClearSavedSSIDs()
{
    // storage.Clear("wifi");
}

esp_err_t Wifi::Deinitialize()
{
    std::lock_guard<std::mutex> _(state_mux);

    state = State::NotInitialized;
    esp_err_t status = esp_wifi_deinit();

    return status;
}

esp_err_t Wifi::Disconnect()
{
    std::lock_guard<std::mutex> _(state_mux);

    state = State::Disconnected;
    esp_err_t status = esp_wifi_disconnect();

    return status;
}

esp_err_t Wifi::ScanNetworks()
{
    std::lock_guard<std::mutex> _(state_mux);

    // Scan the ssids
    state = State::Scanning;

    esp_err_t res = esp_wifi_scan_start(NULL, true);
    if (res != ESP_OK)
        return res;

    wifi_ap_record_t wifi_records[MAX_AP];
    uint16_t num_aps = MAX_AP;

    scanned_aps.clear();
    ap_in_range.clear();
    res = esp_wifi_scan_get_ap_records(&num_aps, wifi_records);
    for (uint16_t i = 0; i < num_aps; ++i)
    {
        std::string str = reinterpret_cast<char*>(wifi_records[i].ssid);
        scanned_aps.push_back(str);

        // Order of strength
        for (uint16_t j = 0; j < credentials.size(); ++j)
        {
            // totally inefficient, but the lists are small so eh
            if (str == credentials[j].ssid)
            {
                ap_in_range.push_back(credentials[j]);
            }
        }
    }

    NET_LOG_INFO("aps in range %d, known aps in range %d", scanned_aps.size(), ap_in_range.size());

    ap_idx = 0;
    return res;
}

void Wifi::Connect()
{
    NET_LOG_INFO("Connect to wifi");
    std::lock_guard<std::mutex> _(state_mux);

    static int32_t last_idx = -1;

    if (scanned_aps.empty())
    {
        NET_LOG_INFO("No scanned aps");
        return;
    }

    if (ap_in_range.empty())
    {
        NET_LOG_INFO("No known aps in range");
        return;
    }

    if (ap_idx >= ap_in_range.size())
    {
        NET_LOG_ERROR("Connect: somehow got a larger ap_idx than what we have");
        abort();
    }

    if (last_idx != ap_idx)
    {
        const ap_cred_t& creds = ap_in_range[ap_idx];

        last_idx = ap_idx;

        // Clear the current values
        memset(wifi_cfg.sta.ssid, 0, sizeof(wifi_cfg.sta.ssid));
        memset(wifi_cfg.sta.password, 0, sizeof(wifi_cfg.sta.password));

        memcpy(wifi_cfg.sta.ssid, creds.ssid.c_str(), creds.ssid.length());

        memcpy(wifi_cfg.sta.password, creds.pwd.c_str(), creds.pwd.length());
    }

    NET_LOG_ERROR("Ap Info: ssid: %s password: %s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    state = State::Connecting;

    NET_LOG_INFO("SetCredentialsTask() Ready to connect state: %i", (int)state);

    ESP_ERROR_CHECK(esp_wifi_connect());
}

esp_err_t Wifi::Initialize()
{
    std::lock_guard<std::mutex> _(state_mux);

    NET_LOG_INFO("Begin initialization:");

    if (state != State::NotInitialized)
    {
        NET_LOG_ERROR("Failed to initialized, not in state NotInitialized but in state: %d",
                      (int)state);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t status = esp_event_loop_create_default();
    ESP_ERROR_CHECK(status);

    // Init the nvs
    NET_LOG_INFO("Init the nvs");
    status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
        ESP_ERROR_CHECK(status);
    }

    NET_LOG_INFO("Init netif");
    status = esp_netif_init();
    ESP_ERROR_CHECK(status);

    if (!esp_netif_create_default_wifi_sta())
    {
        status = ESP_FAIL;
    }
    ESP_ERROR_CHECK(status);

    status = esp_wifi_init(&wifi_init_cfg);
    ESP_ERROR_CHECK(status);
    NET_LOG_INFO("Init complete, status %d", (int)status);

    status = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &EventHandler,
                                                 (void*)this, nullptr);
    ESP_ERROR_CHECK(status);

    status = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &EventHandler,
                                                 (void*)this, nullptr);
    ESP_ERROR_CHECK(status);

    status = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(status);

    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    // status = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    // ESP_ERROR_CHECK(status);

    NET_LOG_INFO("Start wifi");
    status = esp_wifi_start();
    ESP_ERROR_CHECK(status);

    return status;
}

Wifi::State Wifi::GetState() const
{
    return state;
}

bool Wifi::IsConnected() const
{
    return state == State::Connected;
}

bool Wifi::IsInitialized() const
{
    return is_initialized;
}

/** Private functions **/

void Wifi::WifiTask(void* arg)
{
    // The future of this function is as follows:
    // - Try to connect to all APs
    // - Sleep until an event happens
    // - Sleep until a new ssid connect comes in
    // - Sleep for a small amount of time and rescan the networks
    Wifi* wifi = static_cast<Wifi*>(arg);

    while (true)
    {
        switch (wifi->state)
        {
        case State::NotInitialized:
        {
            ESP_ERROR_CHECK(wifi->Initialize());
            break;
        }
        case State::Initialized:
        {
            // Begin trying to connect
            wifi->Connect();
            break;
        }
        case State::ReadyToConnect:
        {
            wifi->Connect();
            break;
        }
        case State::Connecting:
        {
            break;
        }
        case State::WaitingForIP:
        {
            break;
        }
        case State::Connected:
        {
            break;
        }
        case State::InvalidCredentials:
        {
            if (++wifi->ap_idx >= wifi->ap_in_range.size())
            {
                NET_LOG_INFO("All ssids have been tried");

                wifi->state = State::UnableToConnect;
                wifi->ap_idx = 0;
                wifi->retry_timeout = esp_timer_get_time_ms();
            }
            else
            {
                wifi->Connect();
            }
            break;
        }
        case State::Disconnected:
        {
            // User decided to disconnect from wifi, so we don't
            // want to do anything until explicit connecting
            break;
        }
        case State::Error:
        {
            // TODO print some error state, send error to ui
            break;
        }
        case State::UnableToConnect:
        {
            // We'll stay in the state, and try again later.
            if (esp_timer_get_time_ms() - wifi->retry_timeout < 10000)
            {
                break;
            }

            wifi->state = State::ReadyToConnect;
            break;
        }
        case State::Scan:
        {
            ESP_ERROR_CHECK(wifi->ScanNetworks());
            break;
        }
        case State::Scanning:
        {
            break;
        }
        default:
        {
            // Error
            NET_LOG_ERROR("Connect state got into an unknown state, will abort in 1 second");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            abort();
        }
        }

        // Note, if we have no scanned aps, then we want to scan a little more often
        if (wifi->state != State::NotInitialized && wifi->state != State::Error
            && wifi->state != State::Connecting && wifi->state != State::Connected
            && wifi->state != State::WaitingForIP && wifi->state != State::Scan
            && wifi->state != State::Scanning
            && esp_timer_get_time_ms() - wifi->last_ssid_scan > wifi->scan_timeout_ms)
        {
            std::lock_guard<std::mutex> _(wifi->state_mux);
            wifi->prev_state = wifi->state;
            wifi->state = State::Scan;
        }

        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }
}

void Wifi::EventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    Wifi* wifi = static_cast<Wifi*>(arg);

    if (event_base == WIFI_EVENT)
    {
        wifi->WifiEvents(event_id, event_data);
    }
    else if (event_base == IP_EVENT)
    {
        wifi->IpEvents(event_id);
    }
}

inline void Wifi::WifiEvents(int32_t event_id, void* event_data)
{
    const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};
    NET_LOG_INFO("Wifi Event -- event %d", (int)event_id);

    std::lock_guard<std::mutex> _(state_mux);

    switch (event_type)
    {
    case WIFI_EVENT_SCAN_DONE:
    {
        state = prev_state;
        last_ssid_scan = esp_timer_get_time_ms();

        NET_LOG_INFO("Wifi Event- done scanning!");
        break;
    }
    case WIFI_EVENT_STA_START:
    {
        state = State::Initialized;
        is_initialized = true;
        NET_LOG_INFO("Wifi Event - Initialized");
        break;
    }
    case WIFI_EVENT_STA_CONNECTED:
    {
        state = State::WaitingForIP;
        // SendWifiConnectedPacket();
        NET_LOG_INFO("Wifi Event - Waiting for IP");
        break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
    {
        NET_LOG_INFO("Wifi Event - Disconnected");
        // Every time the wifi fails to connect to a network
        // a disconnected event is called
        if (state == State::Connecting)
        {
            if (ap_in_range[ap_idx].attempts++ >= MAX_ATTEMPTS)
            {
                // Reset to ready to connect
                state = State::InvalidCredentials;
                ap_in_range[ap_idx].attempts = 0;
                NET_LOG_ERROR("Max failed attempts, invalid credentials");
            }
            else
            {
                state = State::ReadyToConnect;
            }
        }
        else
        {
            NET_LOG_INFO("Fully disconnected, won't try to reconnect");
            // If we just decide to disconnect the wifi
            state = State::Disconnected;
        }

        break;
    }
    case WIFI_EVENT_STA_STOP:
    {
        state = State::NotInitialized;
        is_initialized = false;

        // This is assuming this is how it works.
        break;
    }
    default:
    {
        NET_LOG_INFO("Unhandled wifi event");
        break;
    }
    }
}

inline void Wifi::IpEvents(int32_t event_id)
{
    NET_LOG_INFO("IP Event - %d", (int)event_id);

    std::lock_guard<std::mutex> _(state_mux);

    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP:
    {
        NET_LOG_INFO("IP Event - Got IP");
        ap_in_range[ap_idx].attempts = 0;
        state = State::Connected;
        break;
    }
    case IP_EVENT_STA_LOST_IP:
    {
        if (state != State::Disconnected)
        {
            state = State::WaitingForIP;
        }
        NET_LOG_INFO("IP Event - Lost IP");
        break;
    }
    default:
    {
        break;
    }
    }
}

#include "wifi.hh"

#include <mutex>
#include <iostream>
#include <algorithm>
#include "logger.hh"

#include "esp_log.h"

#include "../../core/inc/macros.hh"


Wifi::Wifi():
    ap_credentials(),
    ssids_in_range(),
    ssid_idx(0),
    wifi_init_cfg(WIFI_INIT_CONFIG_DEFAULT()),
    wifi_cfg({}),
    state(State::NotInitialized),
    is_initialized(false),
    connect_semaphore(),
    connect_task()
{
    connect_semaphore = xSemaphoreCreateBinary();

    // Start with a value of 1 for readability
    xSemaphoreGive(connect_semaphore);
}

Wifi::~Wifi()
{
    NET_LOG_INFO("Delete wifi");
    vTaskDelete(connect_task);
}

void Wifi::Begin()
{
    xTaskCreate(WifiTask,
        "wifi_connect_task",
        4096,
        (void*)this,
        12,
        &connect_task);
}

const std::vector<std::string>& Wifi::GetNetworksInRange() const
{
    return ssids_in_range;
}

void Wifi::Connect(const std::string& ssid, const std::string& pwd)
{
    ap_cred_t creds = { ssid, pwd, 0 };
    ap_credentials.push_back(std::move(creds));
}

void Wifi::Connect(const char* ssid, const char* password)
{
    Connect(std::string(ssid), std::string(password));
}

void Wifi::Connect(const char* ssid,
    const size_t ssid_len,
    const char* password,
    const size_t password_len)
{
    Connect(std::string(ssid, ssid + ssid_len), std::string(password + password_len));
}

esp_err_t Wifi::Deinitialize()
{
    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY))
    {
        Logger::Log(Logger::Level::Warn, "Deinitialize() Waiting for semaphore");
    }

    state = State::NotInitialized;
    esp_err_t status = esp_wifi_deinit();

    xSemaphoreGive(connect_semaphore);
    return status;
}

esp_err_t Wifi::Disconnect()
{
    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY))
    {
        Logger::Log(Logger::Level::Warn, "Disconnect() Waiting for semaphore");
    }

    state = State::Disconnected;
    esp_err_t status = esp_wifi_disconnect();

    xSemaphoreGive(connect_semaphore);
    return status;
}

esp_err_t Wifi::ScanNetworks()
{
    // TODO lock
    esp_err_t res = esp_wifi_scan_start(NULL, true);
    if (res != ESP_OK) return res;

    wifi_ap_record_t wifi_records[MAX_AP];
    uint16_t num_aps = MAX_AP;

    ssids_in_range.clear();
    res = esp_wifi_scan_get_ap_records(&num_aps, wifi_records);
    for (uint16_t i = 0; i < num_aps; ++i)
    {
        std::string str = (char*)wifi_records[i].ssid;
        ssids_in_range.push_back(str);
    }

    ssid_idx = 0;
    last_ssid_scan = esp_timer_get_time_ms();

    return res;
}

void Wifi::Connect()
{
    static int32_t last_idx = -1;

    if (ap_credentials.size() == 0)
    {
        NET_LOG_INFO("No ap credentials");
        return;
    }

    if (ssid_idx >= ap_credentials.size())
    {
        NET_LOG_ERROR("Connect somehow got a larger ssid_idx than what we have");
        abort();
    }

    if (last_idx != ssid_idx)
    {
        ap_cred_t& creds = ap_credentials[ssid_idx];

        last_idx = ssid_idx;


        // Clear the current values
        memset(wifi_cfg.sta.ssid, 0, sizeof(wifi_cfg.sta.ssid));
        memset(wifi_cfg.sta.password, 0, sizeof(wifi_cfg.sta.password));

        memcpy(wifi_cfg.sta.ssid,
            ap_credentials[ssid_idx].ssid.c_str(),
            ap_credentials[ssid_idx].ssid.length());

        memcpy(wifi_cfg.sta.password,
            ap_credentials[ssid_idx].pwd.c_str(),
            ap_credentials[ssid_idx].pwd.length());
    }

    NET_LOG_ERROR("Ap Info: ssid: %s password: %s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);

    while (!xSemaphoreTake(connect_semaphore, 1000))
    {
        Logger::Log(Logger::Level::Warn, "Connect() Waiting for semaphore");
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    state = State::ReadyToConnect;

    xSemaphoreGive(connect_semaphore);
    NET_LOG_INFO("SetCredentialsTask() Ready to connect state: %i", (int)state);

    ESP_ERROR_CHECK(esp_wifi_connect());
}

esp_err_t Wifi::Initialize()
{

    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY))
    {
        NET_LOG_ERROR("Semaphore should not be in use now????");
    };

    NET_LOG_INFO("Begin initialization:");

    if (state != State::NotInitialized)
    {
        NET_LOG_ERROR("Failed to initialized, not in state NotInitialized but in state: %d", (int)state);
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
    NET_LOG_INFO("Init complete, status %d", (int)state);

    status = esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &EventHandler,
        (void*)this,
        nullptr);
    ESP_ERROR_CHECK(status);

    status = esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &EventHandler,
        (void*)this,
        nullptr);
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

void Wifi::WifiTask(void* params)
{
    // The future of this function is as follows:
    // - Try to connect to all APs
    // - Sleep until an event happens
    // - Sleep until a new ssid connect comes in
    // - Sleep for a small amount of time and rescan the networks
    Wifi* wifi = (Wifi*)params;


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
                // Do nothing
                break;
            }
            case State::WaitingForIP:
            {
                // Do nothing
                break;
            }
            case State::Connected:
            {
                // Do nothing
                break;
            }
            case State::InvalidCredentials:
            {
                if (++wifi->ssid_idx >= wifi->ap_credentials.size())
                {
                    NET_LOG_INFO("All ssids have been tried");

                    if (!xSemaphoreTake(wifi->connect_semaphore, portMAX_DELAY))
                    {
                        NET_LOG_ERROR("InvalidCredential switch failed to get the semaphore, will abort in 1 second");
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                        abort();
                    }

                    wifi->state = State::UnableToConnect;
                    xSemaphoreGive(wifi->connect_semaphore);
                    wifi->ssid_idx = 0;
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
                // User disconnected from wifi
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

                if (!xSemaphoreTake(wifi->connect_semaphore, portMAX_DELAY))
                {
                    NET_LOG_ERROR("UnableToConnect switch failed to get the semaphore, will abort in 1 second");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    abort();
                }

                wifi->state = State::ReadyToConnect;
                xSemaphoreGive(wifi->connect_semaphore);
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

        // NOT IN USE
        if (wifi->state != State::NotInitialized
            && wifi->state != State::Error
            && esp_timer_get_time_ms() - wifi->last_ssid_scan > 8000)
        {
            // Scan the ssids
            ESP_ERROR_CHECK(wifi->ScanNetworks());
        }

        // Delay for a second
        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }
}

void Wifi::EventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Wifi* wifi = (Wifi*)arg;

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
    const wifi_event_t event_type{ static_cast<wifi_event_t>(event_id) };
    NET_LOG_INFO("Wifi Event -- event", (int)event_id);

    switch (event_type)
    {
        case WIFI_EVENT_STA_START:
        {
            state = State::Initialized;
            is_initialized = true;
            NET_LOG_INFO("Wifi Event - Initialized");
            xSemaphoreGive(connect_semaphore);
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            state = State::WaitingForIP;
            // SendWifiConnectedPacket();
            NET_LOG_INFO("Wifi Event - Waiting for IP");
            xSemaphoreGive(connect_semaphore);
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            NET_LOG_INFO("Wifi Event - Disconnected");
            // Every time the wifi fails to connect to a network
            // a disconnected event is called
            if (state == State::Connecting)
            {
                if (ap_credentials[ssid_idx].attempts++ >= MAX_ATTEMPTS)
                {
                    // Reset to ready to connect
                    state = State::InvalidCredentials;
                    ap_credentials[ssid_idx].attempts = 0;
                    NET_LOG_ERROR("Max failed attempts, invalid credentials");
                }
                else
                {
                    state = State::ReadyToConnect;
                }
            }
            else
            {
                // If we just decide to disconnect the wifi
                state = State::Disconnected;
            }

            xSemaphoreGive(connect_semaphore);
            break;
        }
        case WIFI_EVENT_STA_STOP:
        {
            state = State::NotInitialized;
            is_initialized = false;

            // This is assuming this is how it works.
            xSemaphoreGive(connect_semaphore);
            break;
        }
        default:
            break;
    }
}

inline void Wifi::IpEvents(int32_t event_id)
{
    NET_LOG_INFO("IP Event - %d", (int)event_id);

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            NET_LOG_INFO("IP Event - Got IP");
            ap_credentials[ssid_idx].attempts = 0;
            state = State::Connected;
            xSemaphoreGive(connect_semaphore);
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            if (state != State::Disconnected)
            {
                state = State::WaitingForIP;
            }
            NET_LOG_INFO("IP Event - Lost IP");
            xSemaphoreGive(connect_semaphore);
            break;
        }
        default:
        {
            break;
        }
    }
}

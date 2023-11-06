#include "Wifi.hh"

#include <mutex>
#include <iostream>

#include "esp_log.h"

static const char* TAG = "[Net Wifi]";

Wifi* Wifi::instance;
std::recursive_mutex Wifi::mux;

Wifi* Wifi::GetInstance()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    if (instance == nullptr)
    {
        instance = new Wifi();
    }

    return instance;
}


esp_err_t Wifi::Connect()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    logger->info(TAG, "connect()\n");

    // If the config has changed we should ensure that the config is updated
    esp_err_t status{ ESP_OK };

    logger->info(TAG, "connect() state %d \n", state);

    switch (state)
    {
        case State::Connecting:
        case State::WaitingForIP:
        case State::Connected:
        {
            // Disconnect and fall through
            // status = Disconnect();
            // ESP_ERROR_CHECK(status);
            break;
        }
        case State::ReadyToConnect:
        case State::Disconnected:
        {
            status = esp_wifi_connect();
            ESP_ERROR_CHECK(status);
            if (status == ESP_OK)
            {
                std::cout << "moving to connecting state " << std::endl;
                state = State::Connecting;

            }
            break;
        }
        case State::NotInitialized:
        case State::Initialized:
        case State::Error:
            status = ESP_FAIL;
            break;
    }
    return status;
}


esp_err_t Wifi::Connect(const char* ssid, const char* password)
{
    // Set creds, and then call connect
    SetCredentials(ssid, password);
    return Connect();
}

esp_err_t Wifi::Deinitialize()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    state = State::NotInitialized;
    return esp_wifi_deinit();
}

esp_err_t Wifi::Disconnect()
{
    std::lock_guard<std::recursive_mutex> lock(mux);

    state = State::Disconnected;
    return esp_wifi_disconnect();
}

esp_err_t Wifi::ScanNetworks(Vector<String>* ssids)
{
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    esp_err_t res = esp_wifi_scan_start(NULL, true);
    if (res != ESP_OK) return res;

    wifi_ap_record_t wifi_records[MAX_AP];
    uint16_t num_aps = MAX_AP;

    res = esp_wifi_scan_get_ap_records(&num_aps, wifi_records);
    for (uint16_t i = 0; i < num_aps; ++i)
    {
        String str;
        char* ch = (char*)wifi_records[i].ssid;
        while (*ch != '\0')
        {
            str.push_back(*ch);
            ch++;
        }
        ssids->push_back(str);
    }

    return res;
}

esp_err_t Wifi::Initialize()
{
    std::lock_guard<std::recursive_mutex> lock(mux);
    ESP_LOGI(TAG, "begin initialization:");

    if (state != State::NotInitialized)
    {
        ESP_LOGE(TAG, "Failed to initialized, not in state NotInitialized but in state: %d", (int)state);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t status = esp_event_loop_create_default();
    ESP_ERROR_CHECK(status);

    // Init the nvs
    ESP_LOGI(TAG, "Init the nvs");
    status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
        ESP_ERROR_CHECK(status);
    }

    ESP_LOGI(TAG, "Init netif");
    status = esp_netif_init();
    ESP_ERROR_CHECK(status);


    if (!esp_netif_create_default_wifi_sta())
    {
        status = ESP_FAIL;
    }
    ESP_ERROR_CHECK(status);


    status = esp_wifi_init(&wifi_init_cfg);
    ESP_ERROR_CHECK(status);
    ESP_LOGI(TAG, "Init complete, status %d", (int)state);

    status = esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &EventHandler,
        nullptr,
        nullptr);
    ESP_ERROR_CHECK(status);

    status = esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &EventHandler,
        nullptr,
        nullptr);
    ESP_ERROR_CHECK(status);

    status = esp_wifi_set_mode(WIFI_MODE_STA);
    ESP_ERROR_CHECK(status);

    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_cfg.sta.pmf_cfg.capable = true;
    wifi_cfg.sta.pmf_cfg.required = false;

    status = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    ESP_ERROR_CHECK(status);

    ESP_LOGI(TAG, "Start wifi");
    status = esp_wifi_start();
    ESP_ERROR_CHECK(status);

    state = State::ReadyToConnect;

    ESP_LOGI(TAG, "Ready to connect");
    return status;
}

void Wifi::SetCredentials(const char* ssid, const char* password)
{
    // TODO error checking that returns an error if an ssid or password is too long
    memcpy(wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(wifi_cfg.sta.ssid)));
    memcpy(wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(wifi_cfg.sta.password)));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
}

Wifi::State Wifi::GetState() const
{
    return state;
}

bool Wifi::IsConnected() const
{
    return state == State::Connected;
}

/** Private functions **/
void Wifi::EventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Wifi* wifi = Wifi::GetInstance();
    if (event_base == WIFI_EVENT)
    {
        std::cout << "Got Wifi Event: event_id " << event_data << std::endl;
        wifi->WifiEvents(event_id, event_data);
    }
    else if (event_base == IP_EVENT)
    {
        std::cout << "Got IP Event: event_id " << event_data << std::endl;
        wifi->IpEvents(event_id);
    }
}

inline void Wifi::WifiEvents(int32_t event_id, void* event_data)
{
    logger->info(TAG, "WifiEvents- event %d \n", event_id);
    const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};

    switch (event_type)
    {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            state = State::ReadyToConnect;
            std::cout << "Wifi Event - Ready to connect" << std::endl;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            state = State::WaitingForIP;
            std::cout << "Wifi Event - Waiting for IP" << std::endl;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            std::cout << "Wifi Event - Disconnected" << std::endl;
            state = State::Disconnected;
            break;
        }
        default:
            break;
    }
}

inline void Wifi::IpEvents(int32_t event_id)
{
    logger->info(TAG, "IP Event - %d\n\r", event_id);

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            std::cout << "IP Event - Got IP" << std::endl;
            state = State::Connected;
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            if (state != State::Disconnected)
            {
                state = State::WaitingForIP;
            }
            std::cout << "IP Event - Lost IP" << std::endl;
            break;
        }
        default:
            break;
    }
}
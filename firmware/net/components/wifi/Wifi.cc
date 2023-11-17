#include "Wifi.hh"

#include <mutex>
#include <iostream>

#include "esp_log.h"

static const char* TAG = "[net-wifi]";

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

Wifi::Wifi():
    wifi_init_cfg(WIFI_INIT_CONFIG_DEFAULT()),
    wifi_cfg({}),
    state(State::NotInitialized),
    creds_entered(false)
{
    connect_semaphore = xSemaphoreCreateBinary();

    // Start with a value of 1 for readability
    xSemaphoreGive(connect_semaphore);

    ESP_ERROR_CHECK(Initialize());

    xTaskCreate(ConnectTask,
        "wifi_connect_task",
        2048,
        NULL,
        12,
        &event_task);

}

Wifi::~Wifi()
{
    ESP_LOGI(TAG, "Delete wifi");
    vTaskDelete(event_task);
}

void Wifi::Connect(const char* ssid, const char* password)
{
    Connect(ssid,
        std::min(strlen(ssid), sizeof(wifi_cfg.sta.ssid)),
        password,
        std::min(strlen(password), sizeof(wifi_cfg.sta.password))
    );
}

void Wifi::Connect(const char* ssid,
    const size_t ssid_len,
    const char* password,
    const size_t password_len)
{
    memcpy(wifi_cfg.sta.ssid,
        ssid,
        ssid_len);

    memcpy(wifi_cfg.sta.password,
        password,
        password_len);

    ESP_LOGE(TAG, "ssid %s password %s", wifi_cfg.sta.ssid, wifi_cfg.sta.password);

    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY)) { ESP_LOGW(TAG,"Connect() Waiting for semaphore");}

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    state = State::ReadyToConnect;

    xSemaphoreGive(connect_semaphore);
    ESP_LOGI(TAG, "SetCredentialsTask() Ready to connect state: %d", (int)state);
}

esp_err_t Wifi::Deinitialize()
{
    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY)) { ESP_LOGW(TAG,"Deinitialize() Waiting for semaphore"); }

    state = State::NotInitialized;
    esp_err_t status = esp_wifi_deinit();

    xSemaphoreGive(connect_semaphore);
    return status;
}

esp_err_t Wifi::Disconnect()
{
    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY)) {ESP_LOGW(TAG,"Disconnect() Waiting for semaphore"); }

    state = State::Disconnected;
    esp_err_t status = esp_wifi_disconnect();

    xSemaphoreGive(connect_semaphore);
    return status;
}

esp_err_t Wifi::ScanNetworks(Vector<String>* ssids)
{
    // TODO
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

    while (!xSemaphoreTake(connect_semaphore, portMAX_DELAY))
    {
        ESP_LOGE(TAG, "Semaphore should not be in use now????");
    };

    ESP_LOGI(TAG, "Begin initialization:");

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

/** Private functions **/

void Wifi::ConnectTask(void* params)
{
    while (true)
    {
        // Delay for a second
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // If the state is ready to be connected to something then do it.
        if (instance->state != State::ReadyToConnect &&
            instance->state != State::Disconnected) continue;

        // Try to take the semaphore
        while (!xSemaphoreTake(instance->connect_semaphore, portMAX_DELAY)) { ESP_LOGW(TAG,"ConnectTask() Waiting for semaphore"); }

        ESP_LOGI(TAG, "connect() state %d", (int)instance->state);

        esp_err_t status = { ESP_OK };
        ESP_ERROR_CHECK(esp_wifi_connect());
        if (status == ESP_OK)
        {
            ESP_LOGI(TAG, "moving to connecting state");
            instance->state = State::Connecting;
        }

        // Give semaphore in IP events
    }
}

void Wifi::EventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT)
    {
        instance->WifiEvents(event_id, event_data);
    }
    else if (event_base == IP_EVENT)
    {
        instance->IpEvents(event_id);
    }
}

inline void Wifi::WifiEvents(int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Wifi Event -- event %d", (int)event_id);
    const wifi_event_t event_type{ static_cast<wifi_event_t>(event_id) };

    switch (event_type)
    {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            state = State::Initialized;
            ESP_LOGI(TAG, "Wifi Event - Initialized");
            xSemaphoreGive(connect_semaphore);
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            state = State::WaitingForIP;
            ESP_LOGI(TAG, "Wifi Event - Waiting for IP");
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            ESP_LOGI(TAG, "Wifi Event - Disconnected");
            state = State::Disconnected;
            xSemaphoreGive(connect_semaphore);
            break;
        }
        default:
            break;
    }
}

inline void Wifi::IpEvents(int32_t event_id)
{
    ESP_LOGI(TAG, "IP Event - %d", (int)event_id);

    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            ESP_LOGI(TAG, "IP Event - Got IP");
            state = State::Connected;
            xSemaphoreGive(connect_semaphore);
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            std::lock_guard<std::recursive_mutex> state_guard(mux);
            if (state != State::Disconnected)
            {
                state = State::WaitingForIP;
            }
            ESP_LOGI(TAG, "IP Event - Lost IP");
            break;
        }
        default:
            break;
    }
}
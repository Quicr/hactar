#include "wifi.hh"

#include <mutex>
#include <iostream>
#include <algorithm>
#include "logger.hh"

#include "esp_log.h"


Wifi::Wifi():
    wifi_init_cfg(WIFI_INIT_CONFIG_DEFAULT()),
    wifi_cfg({}),
    state(State::NotInitialized),
    failed_attempts(0),
    is_initialized(false),
    connect_semaphore(),
    connect_task()
{
    connect_semaphore = xSemaphoreCreateBinary();

    // Start with a value of 1 for readability
    xSemaphoreGive(connect_semaphore);

    ESP_ERROR_CHECK(Initialize());

    xTaskCreate(ConnectTask,
        "wifi_connect_task",
        4096,
        (void*)this,
        12,
        &connect_task);
}

Wifi::~Wifi()
{
    NET_LOG_INFO("Delete wifi");
    vTaskDelete(connect_task);
}

void Wifi::Connect(const char* ssid, const char* password)
{
    NET_LOG_ERROR("ssid len", std::min(strlen(ssid), (size_t)sizeof(wifi_cfg.sta.ssid)));
    NET_LOG_ERROR("ssid pwd len", std::min(strlen(password), (size_t)sizeof(wifi_cfg.sta.password)));
    Connect(ssid,
        std::min(strlen(ssid), (size_t)sizeof(wifi_cfg.sta.ssid)),
        password,
        std::min(strlen(password), (size_t)sizeof(wifi_cfg.sta.password))
    );
}

void Wifi::Connect(const char* ssid,
    const size_t ssid_len,
    const char* password,
    const size_t password_len)
{
    // Clear the current values
    memset(wifi_cfg.sta.ssid, 0, sizeof(wifi_cfg.sta.ssid));
    memset(wifi_cfg.sta.password, 0, sizeof(wifi_cfg.sta.password));

    memcpy(wifi_cfg.sta.ssid,
        ssid,
        ssid_len);

    memcpy(wifi_cfg.sta.password,
        password,
        password_len);

    NET_LOG_ERROR("ssid: %s password: %s", ssid, password);

    while (!xSemaphoreTake(connect_semaphore, 1000))
    {
        Logger::Log(Logger::Level::Warn, "Connect() Waiting for semaphore");
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    state = State::ReadyToConnect;

    xSemaphoreGive(connect_semaphore);
    NET_LOG_INFO("SetCredentialsTask() Ready to connect state: %i", (int)state);
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

esp_err_t Wifi::ScanNetworks(std::vector<std::string>* ssids)
{
    // TODO
    // wifi_scan_config_t scan_config = {
    //     .ssid = 0,
    //     .bssid = 0,
    //     .channel = 0,
    //     .show_hidden = true
    // };

    esp_err_t res = esp_wifi_scan_start(NULL, true);
    if (res != ESP_OK) return res;

    wifi_ap_record_t wifi_records[MAX_AP];
    uint16_t num_aps = MAX_AP;

    res = esp_wifi_scan_get_ap_records(&num_aps, wifi_records);
    for (uint16_t i = 0; i < num_aps; ++i)
    {
        std::string str = (char*)wifi_records[i].ssid;
        ssids->push_back(str);
    }

    return res;
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

void Wifi::ConnectTask(void* params)
{
    Wifi* instance = (Wifi*)params;

    while (true)
    {
        // Delay for a second
        vTaskDelay(2500 / portTICK_PERIOD_MS);

        // If the state is ready to be connected to something then do it.
        if (instance->state != State::ReadyToConnect)
        {
            continue;
        }

        // Try to take the semaphore
        while (!xSemaphoreTake(instance->connect_semaphore, portMAX_DELAY))
        {
            Logger::Log(Logger::Level::Warn, "ConnectTask() Waiting for semaphore");
        }
        // Give semaphore in IP/HTTP events

        NET_LOG_INFO("connect() state ", (int)instance->state);

        esp_err_t status = { ESP_OK };
        ESP_ERROR_CHECK(esp_wifi_connect());
        if (status == ESP_OK)
        {
            NET_LOG_INFO("moving to connecting state");
            instance->state = State::Connecting;
        }
    }
}

void Wifi::EventHandler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Wifi* instance = (Wifi*)arg;

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
                if (++failed_attempts == MAX_ATTEMPTS)
                {
                    // Reset to ready to connect
                    state = State::InvalidCredentials;
                    failed_attempts = 0;
                    // SendWifiFailedToConnectPacket();
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
                // SendWifiDisconnectPacket();
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
            failed_attempts = 0;
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

// void Wifi::SendWifiConnectedPacket()
// {
//     auto packet = std::make_unique<SerialPacket>(8);

//     // Type
//     packet->SetData(SerialPacket::Types::Command, 0, 1);

//     // Id
//     packet->SetData(serial.NextPacketId(), 1, 2);

//     // Size
//     packet->SetData(3, 3, 2);

//     packet->SetData(SerialPacket::Commands::Wifi, 5, 2);

//     packet->SetData(SerialPacket::WifiTypes::Connect, 7, 1);

//     serial.EnqueuePacket(std::move(packet));
// }

// void Wifi::SendWifiDisconnectPacket()
// {
//     auto packet = std::make_unique<SerialPacket>(8);

//     // Type
//     packet->SetData(SerialPacket::Types::Command, 0, 1);

//     // Id
//     packet->SetData(serial.NextPacketId(), 1, 2);

//     // Size
//     packet->SetData(3, 3, 2);

//     packet->SetData(SerialPacket::Commands::Wifi, 5, 2);

//     packet->SetData(SerialPacket::WifiTypes::Disconnect, 7, 1);

//     serial.EnqueuePacket(std::move(packet));
// }

// void Wifi::SendWifiFailedToConnectPacket()
// {
//     auto packet = std::make_unique<SerialPacket>(8);

//     // Type
//     packet->SetData(SerialPacket::Types::Command, 0, 1);

//     // Id
//     packet->SetData(serial.NextPacketId(), 1, 2);

//     // Size
//     packet->SetData(3, 3, 2);

//     packet->SetData(SerialPacket::Commands::Wifi, 5, 2);

//     packet->SetData(SerialPacket::WifiTypes::FailedToConnect, 7, 1);

//     serial.EnqueuePacket(std::move(packet));
// }

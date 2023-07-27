#include "Wifi.hh"

#include <mutex>

namespace hactar_utils
{

Wifi* Wifi::instance;
std::mutex Wifi::state_mutex;

Wifi* Wifi::GetInstance()
{
    std::lock_guard<std::mutex> lock(state_mutex);
    if (instance == nullptr)
    {
        instance = new Wifi();
    }

    return instance;
}


esp_err_t Wifi::Connect()
{
    // Disconnect has a lock guard
    Disconnect();

    std::lock_guard<std::mutex> connect_guard(state_mutex);

    // If the config has changed we should ensure that the config is updated
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_err_t status{ ESP_OK };

    switch (state)
    {
        case State::ReadyToConnect:
        case State::Disconnected:
        {
            status = esp_wifi_connect();
            ESP_ERROR_CHECK(status);
            if (status == ESP_OK)
            {
                state = State::Connecting;
            }
            break;
        }
        case State::Connecting:
        case State::WaitingForIP:
        case State::Connected:
            break;
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
    std::lock_guard<std::mutex> mutex(state_mutex);
    state = State::NotInitialized;
    return esp_wifi_deinit();
}

esp_err_t Wifi::Disconnect()
{
    std::lock_guard<std::mutex> mutex(state_mutex);

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

    printf("Net - Before scanning network\r\n");
    esp_err_t res = esp_wifi_scan_start(NULL, true);
    printf("Net - After scanning network\r\n");
    if (res != ESP_OK) return res;

    wifi_ap_record_t wifi_records[MAX_AP];
    uint16_t num_aps = MAX_AP;

    printf("Net - Before get ap records\r\n");
    res = esp_wifi_scan_get_ap_records(&num_aps, wifi_records);
    printf("Net - After get ap records, %d\r\n", num_aps);
    for (uint16_t i = 0; i < num_aps; ++i)
    {
        printf("Net - create ap string");
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
    std::lock_guard<std::mutex> mutx_guard(state_mutex);

    if (state != State::NotInitialized)
        return ESP_ERR_INVALID_STATE;

    // Init the nvs
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
    }

    printf("here1\n\r");
    status = esp_netif_init();
    printf("here2\n\r");
    status = esp_event_loop_create_default();
    printf("here3\n\r");

    if (status == ESP_OK)
    {
        if (!esp_netif_create_default_wifi_sta())
        {
            status = ESP_FAIL;
        }
        printf("here4\n\r");
    }

    if (status == ESP_OK)
    {
        status = esp_wifi_init(&wifi_init_cfg);
        printf("here5\n\r");
    }

    if (status == ESP_OK)
    {
        status = esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &EventHandler,
            nullptr,
            nullptr);
        printf("here5\n\r");
    }

    if (status == ESP_OK)
    {
        status = esp_event_handler_instance_register(IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &EventHandler,
            nullptr,
            nullptr);
        printf("here7\n\r");

    }

    if (status == ESP_OK)
    {
        status = esp_wifi_set_mode(WIFI_MODE_STA);
        printf("here8\n\r");

    }

    if (status == ESP_OK)
    {
        wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_cfg.sta.pmf_cfg.capable = true;
        wifi_cfg.sta.pmf_cfg.required = false;

        status = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
        printf("here9\n\r");
    }

    if (status == ESP_OK)
    {
        status = esp_wifi_start();
        printf("here10\n\r");
    }

    if (ESP_OK == status)
    {
        state = State::Initialized;
        printf("here11\n\r");
    }

    return status;
}

void Wifi::SetCredentials(const char* ssid, const char* password)
{
    memcpy(wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(wifi_cfg.sta.ssid)));
    memcpy(wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(wifi_cfg.sta.password)));

    state = State::ReadyToConnect;
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
        printf("Wifi Event\n\r");
        wifi->WifiEvents(event_id, event_data);
    }
    else if (event_base == IP_EVENT)
    {
        printf("IP Event \n\r");
        wifi->IpEvents(event_id);
    }
}

inline void Wifi::WifiEvents(int32_t event_id, void* event_data)
{
    switch (event_id)
    {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::ReadyToConnect;
            printf("NET: Wifi Event - Ready to connect\n\r");
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::WaitingForIP;
            printf("NET: Wifi Event - Waiting for IP\n\r");
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            printf("NET: Wifi Event - Disconnected -- %d\n\r", (int)event_data);
            state = State::Disconnected;
            break;
        }
        default:
            break;
    }
}

inline void Wifi::IpEvents(int32_t event_id)
{
    switch (event_id)
    {
        case IP_EVENT_STA_GOT_IP:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::Connected;
            break;
        }
        case IP_EVENT_STA_LOST_IP:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            if (state != State::Disconnected)
            {
                state = State::WaitingForIP;
            }
            break;
        }
        default:
            break;
    }
}

} // namespace hactar_utils

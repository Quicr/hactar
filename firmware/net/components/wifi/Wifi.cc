#include "Wifi.hh"
#include "Logging.hh"
#include "SerialLogger.hh"

#include <mutex>
#include <iostream>

static const char* TAG = "[wifi]";

namespace hactar_utils
{

static hactar_utils::LogManager* logger = nullptr;
       
Wifi* Wifi::instance;
std::mutex Wifi::state_mutex;

Wifi::Wifi()
{
    logger = hactar_utils::LogManager::GetInstance();
    logger->add_logger(new hactar_utils::ESP32SerialLogger());
    state = State::NotInitialized;
    wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
}
    
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
    
    logger->info(TAG, "connect()\n");    
    
    // Disconnect has a lock guard
    Disconnect();

    std::lock_guard<std::mutex> connect_guard(state_mutex);

    // If the config has changed we should ensure that the config is updated
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_err_t status{ ESP_OK };

    logger->info(TAG, "connect() state %d \n", state);    
    
    switch (state)
    {
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
    std::lock_guard<std::mutex> mutx_guard(state_mutex);
    printf("Wifi::Initialize: State is %d", (int) state);

    if (state != State::NotInitialized) {
        printf("Wifi::Initialize: WHATTTTTT");
        return ESP_ERR_INVALID_STATE;
    }

    // Init the nvs
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        status = nvs_flash_init();
    }

    status = esp_netif_init();
    status = esp_event_loop_create_default();

    if (status == ESP_OK)
    {
        if (!esp_netif_create_default_wifi_sta())
        {
            status = ESP_FAIL;
        }
    }

    if (status == ESP_OK)
    {
        status = esp_wifi_init(&wifi_init_cfg);
        std::cout << "wifi::init:: esp_wifi_init done, status " << status << std::endl;

    }


    if (status == ESP_OK)
    {
        status = esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &EventHandler,
            nullptr,
            nullptr);
    }

    if (status == ESP_OK)
    {
        status = esp_event_handler_instance_register(IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &EventHandler,
            nullptr,
            nullptr);
    }

    if (status == ESP_OK)
    {
        status = esp_wifi_set_mode(WIFI_MODE_STA);
    }

    if (status == ESP_OK)
    {
        wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_cfg.sta.pmf_cfg.capable = true;
        wifi_cfg.sta.pmf_cfg.required = false;

        status = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    }


    if (status == ESP_OK)
    {
        std::cout << "wifi::init:: doing esp_wifi_start now " << std::endl;
        status = esp_wifi_start();
    }

    if (ESP_OK == status)
    {
        state = State::Initialized;
    }

    std::cout << "wifi::init:: final status " << status << std::endl;

    return status;
}

void Wifi::SetCredentials(const char* ssid, const char* password)
{
    std::string sssid = "ramanujan";
    std::string spassword = "JaiGanesha!23";
    memcpy(wifi_cfg.sta.ssid, sssid.c_str(), sssid.size());
    memcpy(wifi_cfg.sta.password, spassword.c_str(), spassword.size()); 
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
    //logger->info(TAG, "WifiEvents- event %d \n", event_id);    
    
    switch (event_id)
    {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::ReadyToConnect;
            std::cout << "Wifi Event - Ready to connect" << std::endl;
            break;
        }
        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::WaitingForIP;
            std::cout << "Wifi Event - Waiting for IP" << std::endl;
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
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
            std::lock_guard<std::mutex> state_guard(state_mutex);
            std::cout << "IP Event - Got IP" << std::endl;
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
            std::cout <<  "IP Event - Lost IP" << std::endl;
            break;
        }
        default:
            break;
    }
}

} // namespace hactar_utils

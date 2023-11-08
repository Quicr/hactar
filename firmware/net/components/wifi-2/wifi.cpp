#include "wifi.h"

#include <iostream>

namespace net_wifi
{
std::mutex Wifi::state_mutex{};
Wifi::State Wifi::state{State::NotInitialized};
wifi_init_config_t Wifi::wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
wifi_config_t Wifi::wifi_cfg{};

void Wifi::wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};
        std::cout << "wifi_event_handler: got wifi-event " << (int) event_type << std::endl;
        switch (event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::ReadyToConnect;
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::WaitingForIP;
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            std::lock_guard<std::mutex> state_guard(state_mutex);
            state = State::Disconnected;
            break;
        }

        default:
            break;
        }
    }
}

void Wifi::ip_event_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT)
    {
        const ip_event_t event_type{static_cast<ip_event_t>(event_id)};
        std::cout << "ip_event_handler: got ip-event " << (int) event_type  << std::endl;

        switch (event_type)
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
}

esp_err_t Wifi::connect()
{
    std::lock_guard<std::mutex> connect_guard(state_mutex);

    esp_err_t status{ESP_OK};

    switch (state)
    {
    case State::ReadyToConnect:
    case State::Disconnected:
        status = esp_wifi_connect();
        if (status == ESP_OK)
        {
            state = State::Connecting;
        }
        break;
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
    std::cout << "Wifi: connect(): State " << int(state) << " Status " << esp_err_to_name(status) << std::endl;
    return status;
}

esp_err_t Wifi::initialize()
{
    std::lock_guard<std::mutex> mutx_guard(state_mutex);

    esp_err_t status{ESP_OK};

    if (state == State::NotInitialized)
    {
        status |= esp_netif_init();
        if (status == ESP_OK)
        {
            std::cout << "Wifi: 1" << std::endl;
            if(!esp_netif_create_default_wifi_sta()) {
                status = ESP_FAIL;
            }
        }

        if (status == ESP_OK)
        {
            status = esp_wifi_init(&wifi_init_cfg);
            std::cout << "Wifi: 2, Status " << status  << std::endl;
        }

        if (status == ESP_OK)
        {
            status = esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler,
                                                         nullptr,
                                                         nullptr);
           
           std::cout << "Wifi: 3, Status " << esp_err_to_name(status) << std::endl;                                            
        }

        if (status == ESP_OK)
        {
            status = esp_event_handler_instance_register(IP_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &ip_event_handler,
                                                         nullptr,
                                                         nullptr);
            std::cout << "Wifi: 4" << std::endl;
        }

        if (status == ESP_OK)
        {
            status = esp_wifi_set_mode(WIFI_MODE_STA);
            std::cout << "Wifi: 5" << std::endl;
        }

        if (status == ESP_OK)
        {
            wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_cfg.sta.pmf_cfg.capable = true;
            wifi_cfg.sta.pmf_cfg.required = false;

            status = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
            std::cout << "Wifi: 6" << std::endl;
        }

        if (status == ESP_OK)
        {
            status = esp_wifi_start();
            std::cout << "Wifi: 7" << std::endl;
        }

        if (ESP_OK == status)
        {
            state = State::Initialized;
            std::cout << "Wifi: 8" << std::endl;
        }
    }
    else if (state == State::Error)
    {
        state = State::NotInitialized;
        std::cout << "Wifi: Noooo" << std::endl;
    }

    std::cout << "WIFI:initialize: Status " << (int) status 
              << " State " << (int) state << std::endl;
    return status;
}


void Wifi::SetCredentials(const char *ssid, const char *password)
{
    memcpy(wifi_cfg.sta.ssid, ssid, std::min(strlen(ssid), sizeof(wifi_cfg.sta.ssid)));

    memcpy(wifi_cfg.sta.password, password, std::min(strlen(password), sizeof(wifi_cfg.sta.password)));
}

esp_err_t Wifi::init()
{
    return initialize();
}


} // namespace WIFI
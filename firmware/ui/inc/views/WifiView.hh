#pragma once

#include "ViewInterface.hh"
#include <string>

class WifiView : public ViewInterface
{
public:
    WifiView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~WifiView();
protected:
    void Update();
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void HandleWifiInput();

private:
    void SendGetSSIDPacket();

    enum WifiState {
        SSID,
        Password,
        Connecting
    };


    int8_t last_num_ssids;
    uint32_t next_get_ssid_timeout;
    WifiState state;
    std::string request_msg;
    std::string ssid;
    std::map<uint8_t, std::string> ssids;
    std::string password;
    uint32_t state_update_timeout;
    uint16_t num_connection_checks;
    bool connecting_done;
};
#pragma once

#include "ViewInterface.hh"
#include <string>

class FirstBootView : public ViewInterface
{
public:
    FirstBootView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager,
             SerialPacketManager& serial,
             Network& network);
    ~FirstBootView();

protected:
    enum class State {
        Username,
        Passcode,
        Wifi,
        Final
    };

    enum class WifiState {
        SSID,
        Password,
        Connecting,
        Connected
    };

    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update();

private:
    // Input functions
    void SetWifi();
    void SetAllDefaults();

    void DrawSSIDs();

    void SetSSID();

    // Update functions
    void UpdateConnecting();

    State state;
    std::string request_message;
    WifiState wifi_state;
    const std::map<uint8_t, std::string>* ssids;
    std::string ssid;
    std::string password;
    uint32_t state_update_timeout;
    uint8_t num_connection_checks;
};
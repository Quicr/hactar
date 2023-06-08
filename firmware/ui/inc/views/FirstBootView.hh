#pragma once

#include "ViewInterface.hh"
#include "String.hh"

class FirstBootView : public ViewInterface
{
public:
    FirstBootView(UserInterfaceManager& manager,
                  Screen& screen,
                  Q10Keyboard& keyboard,
                  SettingManager& setting_manager);
    ~FirstBootView();

protected:
    enum State {
        Username,
        Passcode,
        Wifi,
        Final
    };

    enum WifiState {
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

    // Update functions
    void UpdateConnecting();

    State state;
    String request_message;
    WifiState wifi_state;
    String ssid;
    String password;
    uint32_t state_update_timeout;
    uint8_t num_connection_checks;
};
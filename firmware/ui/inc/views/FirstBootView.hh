#pragma once

#include "ViewBase.hh"
#include "String.hh"

class FirstBootView : public ViewBase
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
    bool HandleInput();

private:
    void SetWifi();
    void SetFinal();
    void SetAllDefaults();

    State state;
    String request_message;
    WifiState wifi_state;
    String ssid;
    String password;
};
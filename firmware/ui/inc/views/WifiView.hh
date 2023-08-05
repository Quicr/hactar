#pragma once

#include "ViewInterface.hh"
#include "String.hh"

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

private:
    void SendGetSSIDPacket();

    uint8_t last_num_ssids;
    uint32_t next_get_ssid_timeout;
};
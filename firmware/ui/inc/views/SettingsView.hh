#pragma once

#include "ViewInterface.hh"

class SettingsView : public ViewInterface
{
public:
    SettingsView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio);
    ~SettingsView();
protected:
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update(uint32_t current_tick);
};
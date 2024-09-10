#pragma once

#include "view_interface.hh"
#include <string>

class TeamView : public ViewInterface
{
public:
    TeamView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio);
    ~TeamView();
protected:
    void Update(uint32_t current_tick);
    void AnimatedDraw();
    void Draw();
    void HandleInput();
};
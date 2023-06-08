#pragma once

#include "ViewInterface.hh"

class SettingsView : public ViewInterface
{
public:
    SettingsView(UserInterfaceManager& manager,
                 Screen& screen,
                 Q10Keyboard& keyboard,
                 SettingManager& setting_manager);
    ~SettingsView();
protected:
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update();
};
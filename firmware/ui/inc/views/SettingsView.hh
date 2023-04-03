#pragma once

#include "ViewBase.hh"

class SettingsView : public ViewBase
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
    bool HandleInput();
};
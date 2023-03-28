#pragma once

#include "ViewBase.hh"

class SettingsView : public ViewBase
{
public:
    SettingsView(UserInterfaceManager& manager,
                 Screen& screen,
                 Q10Keyboard& keyboard,
                 EEPROM& eeprom);
    ~SettingsView();
protected:
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
};
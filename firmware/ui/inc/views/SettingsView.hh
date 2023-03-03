#pragma once

#include "ViewBase.hh"

class SettingsView : public ViewBase
{
public:
    SettingsView(UserInterfaceManager &manager,
                 Screen &screen,
                 Q10Keyboard &keyboard);
    ~SettingsView();
protected:
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
};
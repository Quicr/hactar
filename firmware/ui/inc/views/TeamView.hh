#pragma once

#include "ViewBase.hh"
#include "String.hh"

class TeamView : public ViewBase
{
public:
    TeamView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~TeamView();
protected:
    void Update();
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
};
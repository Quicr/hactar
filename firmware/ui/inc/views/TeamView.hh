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
    bool Update();
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
};
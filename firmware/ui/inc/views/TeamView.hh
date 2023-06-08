#pragma once

#include "ViewInterface.hh"
#include "String.hh"

class TeamView : public ViewInterface
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
    void HandleInput();
};
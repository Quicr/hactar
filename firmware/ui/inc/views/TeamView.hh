#pragma once

#include "ViewBase.hh"
#include "String.hh"

class TeamView : public ViewBase
{
public:
    TeamView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             EEPROM& eeprom);
    ~TeamView();
protected:
    void Get();
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
};
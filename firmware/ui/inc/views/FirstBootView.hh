#pragma once

#include "ViewBase.hh"
#include "String.hh"

class FirstBootView : public ViewBase
{
public:
    FirstBootView(UserInterfaceManager& manager,
                  Screen& screen,
                  Q10Keyboard& keyboard,
                  SettingManager& settings);
    ~FirstBootView();

protected:
    void Draw();
    bool HandleInput();

private:
    void SetPasscode(String pass);
};
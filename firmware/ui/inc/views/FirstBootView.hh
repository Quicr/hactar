#pragma once

#include "ViewBase.hh"
#include "String.hh"

class FirstBootView : public ViewBase
{
public:
    FirstBootView(UserInterfaceManager& manager,
                  Screen& screen,
                  Q10Keyboard& keyboard,
                  EEPROM& eeprom);
    ~FirstBootView();

protected:
    enum State {
        Username,
        Passcode,
        Wifi,
        Final
    };

    void AnimatedDraw();
    void Draw();
    bool HandleInput();

private:
    void SetUsername();
    void SetPasscode();
    void SetWifi();
    void SetFinal();
    void SetAllDefaults();

    State state;
};
#pragma once

#include "ViewBase.hh"
#include "String.hh"

class LoginView : public ViewBase
{
public:
    LoginView(UserInterfaceManager& manager,
              Screen& screen,
              Q10Keyboard& keyboard,
              EEPROM& eeprom);
    ~LoginView();
protected:
    void AnimatedDraw();
    void Draw();
    bool HandleInput();

private:
    void DrawFirstLoad();
    void DrawIncorrectPasscode();
    // TODO load from the EEPROM
    String passcode = "";
    bool incorrect_passcode_entered;
};
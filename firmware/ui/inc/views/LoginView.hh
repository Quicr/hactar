#pragma once

#include "ViewInterface.hh"
#include "String.hh"

class LoginView : public ViewInterface
{
public:
    LoginView(UserInterfaceManager& manager,
              Screen& screen,
              Q10Keyboard& keyboard,
              SettingManager& setting_manager);
    ~LoginView();
protected:
    void AnimatedDraw();
    void Draw();
    bool HandleInput();
    bool Update();

private:
    void DrawFirstLoad();
    void DrawIncorrectPasscode();
    bool incorrect_passcode_entered;
};
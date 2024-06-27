#pragma once

#include "ViewInterface.hh"
#include <string>

class LoginView : public ViewInterface
{
public:
    LoginView(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio);
    ~LoginView();
protected:
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update();

private:
    /** Private functions **/
    void DrawFirstLoad();
    void DrawIncorrectPasscode();

    /** Private variables **/
    bool incorrect_passcode_entered;
};
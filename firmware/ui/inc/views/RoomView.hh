#pragma once

#include "ViewInterface.hh"

class RoomView : public ViewInterface
{
public:
    RoomView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~RoomView();
protected:
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update();

private:

};

#pragma once

#include "screen.hh"

class Graphics
{
public:
    Graphics(Screen* screen);
    ~Graphics();

    void Push();
    void Pop();
private:
    Screen* screen;
};
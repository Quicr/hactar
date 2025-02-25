#pragma once

#include "screen.hh"

class Renderer
{
public:
    enum class View : uint8_t
    {
        Startup = 0,

    };

    Renderer(Screen& screen);

    void Render() noexcept;
private:
    void StartupView();

    Screen& screen;
    View view;
};
#include "graphics.hh"

Graphics::Graphics(Screen* screen) : screen(screen)
{

}

Graphics::~Graphics()
{
    screen = nullptr;
}
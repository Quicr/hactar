#include "renderer.hh"

Renderer::Renderer(Screen& screen) :
    screen(screen),
    view(View::Startup)
{

}

void Renderer::Render()
{
    switch (view)
    {
        case View::Startup:
        {

        }
        default:
        {
            Error("Renderer render", "Given view is not handled");
        }
    }
}
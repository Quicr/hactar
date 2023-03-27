#include "FirstBootView.hh"

FirstBootView::FirstBootView(UserInterfaceManager& manager,
                             Screen& screen,
                             Q10Keyboard& keyboard,
                             SettingManager& settings) :
    ViewBase(manager, screen, keyboard, settings)
{
    // Clear the whole eeprom
    settings.ClearSettings();

    settings.RegisterSetting(0, 0);
}

void FirstBootView::Draw()
{

}

bool FirstBootView::HandleInput()
{

}
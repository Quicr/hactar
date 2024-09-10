#include "error_manager.hh"

ErrorManager::ErrorManager(Screen& _screen) :
    screen(&_screen)
{
}

ErrorManager::~ErrorManager()
{

}


void ErrorManager::RaiseError(const uint32_t error_code)
{
    UNUSED(error_code);
}

#pragma once

#include "Screen.hh"
#include <vector>

class ErrorManager
{
public:
    ErrorManager(Screen& _screen);
    ~ErrorManager();
    void RaiseError(const uint32_t error_code);
    std::vector<uint32_t>& GetErrors();

private:
    Screen* screen;
    std::vector<uint32_t> errors_log;
};
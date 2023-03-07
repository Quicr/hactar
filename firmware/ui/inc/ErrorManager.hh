#pragma once

#include "Screen.hh"
#include "Vector.hh"

class ErrorManager
{
public:
    ErrorManager(Screen& screen);
    ~ErrorManager();
    void RaiseError(const uint32_t error_code);
    Vector<uint32_t>& GetErrors();

private:
    Screen* screen;
    Vector<uint32_t> errors_log;
};
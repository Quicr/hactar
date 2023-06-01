#pragma once

#include <map>
#include <functional>
#include "Vector.hh"
#include "String.hh"

#include "UserInterfaceManager.hh"


class CommandHandler
{
public:
    CommandHandler(UserInterfaceManager* manager);

    bool ChangeViewCommand(const String& command);
    bool ReadCommand(String cmd);

private:
    UserInterfaceManager* manager;
};
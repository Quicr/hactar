#pragma once

#include <map>
#include <functional>
#include <vector>
#include <string>

#include "user_interface_manager.hh"

// TODO rename to view changer
class CommandHandler
{
public:
    CommandHandler(UserInterfaceManager* manager);

    bool ChangeViewCommand(const std::string& command);
    bool ReadCommand(std::string cmd);

private:
    UserInterfaceManager* manager;
};
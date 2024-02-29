#include "CommandHandler.hh"

#include "ChatView.hh"
#include "FirstBootView.hh"
#include "LoginView.hh"
#include "RoomView.hh"
#include "SettingsView.hh"
#include "TeamView.hh"
#include "WifiView.hh"

CommandHandler::CommandHandler(UserInterfaceManager* manager) :
    manager(manager)
{

}

bool CommandHandler::ChangeViewCommand(const std::string& command)
{
    if (command == "/login")
        return manager->ChangeView<LoginView>();
    else if (command == "/chat")
        return manager->ChangeView<ChatView>();
    else if (command == "/rooms")
        return manager->ChangeView<RoomView>();
    else if (command == "/t")
        return manager->ChangeView<TeamView>();
    else if (command == "/s")
        return manager->ChangeView<SettingsView>();
    else if (command == "/wifi")
        return manager->ChangeView<WifiView>();
    else if (command == "/hardreset")
        return manager->ChangeView<FirstBootView>();

    return false;
}
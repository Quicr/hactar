#include "CommandHandler.hh"


#include "ChatView.hh"
#include "LoginView.hh"
#include "TeamView.hh"
#include "SettingsView.hh"

CommandHandler::CommandHandler(UserInterfaceManager* manager) :
    manager(manager)
{

}

bool CommandHandler::ChangeViewCommand(const String& command)
{
    if (command == "/login")
        return manager->ChangeView<LoginView>();
    else if (command == "/chat")
        return manager->ChangeView<ChatView>();
    // else if (command == "/wifi")
    //     return manager->ChangeView<ChatView>();
    else if (command == "/t")
        return manager->ChangeView<TeamView>();
    else if (command == "/s")
        return manager->ChangeView<SettingsView>();

    return false;
}
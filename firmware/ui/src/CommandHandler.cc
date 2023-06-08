#include "CommandHandler.hh"


#include "TeamView.hh"
#include "SettingsView.hh"
#include "ChatView.hh"

CommandHandler::CommandHandler(UserInterfaceManager* manager) :
    manager(manager)
{

}

bool CommandHandler::ChangeViewCommand(const String& command)
{
    if (command == "/t")
        return manager->ChangeView<TeamView>();
    else if (command == "/s")
        return manager->ChangeView<SettingsView>();
    else if (command == "/chat")
        return manager->ChangeView<ChatView>();

    return false;
}
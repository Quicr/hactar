#include "command_handler.hh"

#include "chat_view.hh"
#include "first_boot_view.hh"
#include "login_view.hh"
#include "room_view.hh"
#include "settings_view.hh"
#include "team_view.hh"
#include "wifi_view.hh"

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
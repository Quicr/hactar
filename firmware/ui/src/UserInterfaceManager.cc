#include "UserInterfaceManager.hh"
#include "String.hh"
#include "LoginView.hh"
#include "ChatView.hh"
#include "TeamView.hh"

UserInterfaceManager::UserInterfaceManager(Screen &screen,
                                           Q10Keyboard &keyboard,
                                           SerialManager &net_layer) :
    screen(&screen),
    keyboard(&keyboard),
    net_layer(&net_layer),
    view(nullptr),
    received_messages(),
    send_packets(),
    force_redraw(false),
    current_time(HAL_GetTick()),
    next_message_receive_timeout(0),
    next_message_transmit_timeout(0)
{
    ChangeView<LoginView>();
}

UserInterfaceManager::~UserInterfaceManager()
{
    // TODO note - Do we need to delete the next 3?
    // Probably not since they are allocated outside of this class
    // However it does manage it
    delete screen;
    delete keyboard;
    delete net_layer;
    delete view;
}

// TODO should update this to be a draw/update architecture
void UserInterfaceManager::Run()
{
    current_time = HAL_GetTick();

    view->Run();

    if (RedrawForced())
    {
        force_redraw = false;
        return;
    }
    // TODO we probably should keep a small list of the most recent messages
    //      in the user interface manager instead of chat view
    //      otherwise it will be bizarre having to get all of the old messages.
    // TODO this should only occur in the chat view mode?
    SendSerialMessages();
    GetSerialMessages();
}

bool UserInterfaceManager::HasMessages()
{
    return received_messages.size();
}

Vector<Message>& UserInterfaceManager::GetMessages()
{
    return received_messages;
}

void UserInterfaceManager::ClearMessages()
{
    received_messages.clear();
}

void UserInterfaceManager::EnqueuePacket(Packet& packet)
{
    // TODO maybe make this into a linked list?
    send_packets.push_back(std::move(packet));
}

void UserInterfaceManager::ForceRedraw()
{
    force_redraw = true;
    // TODO change this to draw
    Run();
}

bool UserInterfaceManager::RedrawForced()
{
    return force_redraw;
}

void UserInterfaceManager::SendSerialMessages()
{
    // TODO keep track of packets until they are successful in transferring
    unsigned int data_size = 0;

    if (send_packets.size() > 0)
    {
        // Linked queue would be better for this.
        // Send a packet
        SerialManager::SerialStatus status = net_layer->WriteSerialInterrupt(
            send_packets[0], current_time);

        view->SetTxColour(GetStatusColour(status));

        send_packets.erase(0);
    }

    // DELETE this is temporary
    send_packets.clear();

}

void UserInterfaceManager::GetSerialMessages()
{
    SerialManager::SerialStatus status =
        net_layer->ReadSerialInterrupt(current_time);

    // Set the colour if its not empty because it will reset its colour
    // after a short timeout
    if (status != SerialManager::SerialStatus::EMPTY)
        view->SetRxColour(GetStatusColour(status));

    // TODO need to have some sort of error?
    // Check the status
    if (status != SerialManager::SerialStatus::OK) return;

    // Get the packets
    Vector<Packet*>& packets = net_layer->GetPackets();

    for (uint32_t i = 0; i < packets.size(); ++i)
    {
        // Write a message to the screen
        Message in_msg;
        // TODO The message should be parsed some how here.
        in_msg.Timestamp("00:00");
        in_msg.Sender("Server");

        String body;

        // Skip the type and length, add the whole message
        unsigned short packet_len = packets[i]->GetData(14, 10);
        for (uint32_t j = 0; j < packet_len; ++j)
        {
            body.push_back((char)packets[i]->GetData(24 + (j * 8), 8));
        }

        in_msg.Body(body);
        received_messages.push_back(in_msg);
    }

    // THINK Is this legal?
    packets.clear();
}

const uint32_t UserInterfaceManager::GetStatusColour(
    const SerialManager::SerialStatus status) const
{
    if (status == SerialManager::SerialStatus::OK)
        return C_GREEN;
    else if (status == SerialManager::SerialStatus::PARTIAL)
        return C_CYAN;
    else if (status == SerialManager::SerialStatus::TIMEOUT)
        return C_MAGENTA;
    else if (status == SerialManager::SerialStatus::BUSY)
        return C_YELLOW;
    else if (status == SerialManager::SerialStatus::ERROR)
        return C_RED;
    else
        return C_WHITE;
}

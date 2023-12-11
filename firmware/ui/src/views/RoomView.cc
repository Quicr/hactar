#include "RoomView.hh"

RoomView::RoomView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager):
    ViewInterface(manager, screen, keyboard, setting_manager),
    selected_room_id(-1),
    connecting_to_room(false),
    next_get_rooms_time(0)
{

}

RoomView::~RoomView()
{

}

void RoomView::AnimatedDraw()
{

}

void RoomView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            // TODO should this go into the constructor?
            // screen.FillRectangle(0, 13, screen.ViewWidth(), 14, fg);
            screen.DrawText(0, 1, "Chat Rooms", menu_font, fg, bg);
            first_load = false;
        }

        screen.FillRectangle(1,
            screen.ViewHeight() - (usr_font.height * 3), screen.ViewWidth(), screen.ViewHeight() - (usr_font.height * 2), bg);
        redraw_menu = false;
    }
}

void RoomView::HandleInput()
{
    // Parse commands
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);

        if (usr_input == "/refresh" || usr_input == "/r")
        {

        }

        return;
    }

    SelectRoom();
}

void RoomView::Update()
{
    // Occasionally request for the current rooms
    if (HAL_GetTick() >= next_get_rooms_time)
    {
        RequestRooms();
    }

}

void RoomView::RequestRooms()
{
    next_get_rooms_time = HAL_GetTick() + 5000;

    // Send request to the esp32 to get the rooms
    std::unique_ptr<Packet> room_req_packet = std::make_unique<Packet>();
    room_req_packet->SetData(Packet::Types::Command, 0, 6);
    room_req_packet->SetData(manager.NextPacketId(), 6, 8);
    room_req_packet->SetData(1, 14, 10);
    room_req_packet->SetData(Packet::Commands::RoomsGet, 24, 8);
    manager.EnqueuePacket(std::move(room_req_packet));
}

void RoomView::DisplayRooms()
{

}

void RoomView::SelectRoom()
{

}

void RoomView::ConnectToRoom()
{

}
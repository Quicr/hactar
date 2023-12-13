#include "RoomView.hh"

#include "TitleBar.hh"

RoomView::RoomView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager):
    ViewInterface(manager, screen, keyboard, setting_manager),
    selected_room_id(-1),
    connecting_to_room(false),
    next_get_rooms_time(0),
    rooms(),
    last_num_rooms(0)
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
            DrawTitleBar("Rooms", menu_font, C_WHITE, C_BLACK, screen);
            screen.FillRectangle(0, screen.ViewHeight() - usr_font.height - 3, screen.ViewWidth(), screen.ViewHeight() - usr_font.height - 2, fg);

            first_load = false;
        }

        screen.FillRectangle(1,
            screen.ViewHeight() - (usr_font.height * 3), screen.ViewWidth(), screen.ViewHeight() - (usr_font.height * 2), bg);
        screen.DrawText(1, screen.ViewHeight() - (usr_font.height * 3),
            request_msg, usr_font, fg, bg);

        redraw_menu = false;
    }

    // TODO move into DrawInputString?? Override if needed?
    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        ViewInterface::DrawInputString(draw_str);
    }

    DisplayRooms();
}

void RoomView::HandleInput()
{
    // Parse commands
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);



        return;
    }

    SelectRoom();
}

void RoomView::Update()
{
    uint32_t current_tick = HAL_GetTick();
    // Occasionally request for the current rooms
    if (HAL_GetTick() >= next_get_rooms_time)
    {
        RequestRooms();
        rooms.clear();
        next_get_rooms_time = current_tick + 60000;
    }

    if (current_tick > state_update_timeout)
    {
        RingBuffer<std::unique_ptr<Packet>>* room_packets;
        if (manager.GetReadyPackets(&room_packets, Packet::Commands::RoomsGet))
        {
            while (room_packets->Unread() > 0)
            {
                // Parse the packets for the room
                auto rx_packet = std::move(room_packets->Read());

                // Build the string that will be the room
                qchat::Room room;
                qchat::Codec::decode(room, rx_packet, 32);

                rooms[rooms.size()+1] = room;
            }
        }
        state_update_timeout = current_tick + 500;
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
    if (last_num_rooms == rooms.size())
    {
        return;
    }

    const uint16_t y_start = 50;
    // Clear the area for the rooms
    screen.FillRectangle(0, y_start, screen.ViewWidth(), screen.ViewHeight() - 100, C_BLACK);

    uint16_t idx = 0;
    for (auto room : rooms)
    {
        // Convert the room id into a string
        const String room_id_str = String::int_to_string(room.first);

        screen.DrawText(1, y_start + (idx * usr_font.height),
            room_id_str, usr_font, C_WHITE, C_BLACK);
        screen.DrawText(1 + usr_font.width, y_start + (idx * usr_font.height),
            ".", usr_font, C_WHITE, C_BLACK);

        screen.DrawText(1 + usr_font.width * 3, y_start + (idx * usr_font.height),
            room.second.friendly_name.c_str(), usr_font, C_WHITE, C_BLACK);
        ++idx;
    }

    last_num_rooms = rooms.size();
}

void RoomView::SelectRoom()
{
    // Read the value that was entered by the user
    int32_t room_id = usr_input.ToNumber();

    if (room_id == -1)
    {
        request_msg = "Error. Please select a room number";
        redraw_menu = true;
        return;
    }

    if (rooms.find(room_id) == rooms.end())
    {
        request_msg = "Error. Please select a room number";
        redraw_menu = true;
        return;
    }

    ConnectToRoom(rooms[room_id]);
}

void RoomView::ConnectToRoom(qchat::Room room)
{
    // TODO placeholder for better more suitable code
    // TODO Active room needs to be passed to the chat view some how
    std::string user_name{ setting_manager.Username()->c_str() };

    // Set watch on the room
    qchat::WatchRoom watch = qchat::WatchRoom{
        .publisher_uri = room.publisher_uri,
        .room_uri = room.room_uri,
    };

    std::unique_ptr<Packet> packet = std::make_unique<Packet>(HAL_GetTick(), 1);
    packet->SetData(Packet::Types::Message, 0, 6);
    packet->SetData(manager.NextPacketId(), 6, 8);

    qchat::Codec::encode(packet, watch);
    uint64_t new_offset = packet->BitsUsed();

    // Expiry time
    packet->SetData(0xFFFFFFFF, new_offset, 32);
    new_offset += 32;

    // Creation time
    packet->SetData(0, new_offset, 32);
    // new_offset += 32;
    manager.EnqueuePacket(std::move(packet));

    ChangeView("/chat");
}
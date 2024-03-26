#pragma once

#include "ViewInterface.hh"

class RoomView : public ViewInterface
{
public:
    RoomView(UserInterfaceManager& manager,
             Screen& screen,
             Q10Keyboard& keyboard,
             SettingManager& setting_manager);
    ~RoomView();
protected:
    void AnimatedDraw();
    void Draw();
    void HandleInput();
    void Update();

private:
    void RequestRooms();
    void DisplayRooms();
    void SelectRoom();
    void ConnectToRoom(std::unique_ptr<qchat::Room> room);


    int32_t selected_room_id;
    bool connecting_to_room;
    uint32_t next_get_rooms_time;
    uint32_t state_update_timeout;
    std::map<uint32_t, std::unique_ptr<qchat::Room>> rooms;
    uint32_t last_num_rooms;

    // UI variables
    std::string request_msg;
};

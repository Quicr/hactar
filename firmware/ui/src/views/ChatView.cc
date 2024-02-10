
#include "ChatView.hh"
#include "UserInterfaceManager.hh"
#include "TeamView.hh"
#include "SettingsView.hh"
#include "QChat.hh"
#include <string>

#include "TitleBar.hh"

ChatView::ChatView(UserInterfaceManager& manager,
    Screen& screen,
    Q10Keyboard& keyboard,
    SettingManager& setting_manager):
    ViewInterface(manager, screen, keyboard, setting_manager)
{
    redraw_messages = true;
}

ChatView::~ChatView()
{

}

void ChatView::Update()
{
}

void ChatView::AnimatedDraw()
{

}

void ChatView::Draw()
{
    ViewInterface::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            DrawUsrInputSeperator();
            // DrawTitle();
            DrawTitleBar(manager.ActiveRoom()->friendly_name.c_str(),
                menu_font, C_WHITE, C_BLACK, screen);
            first_load = false;
        }
    }

    // TODO move into ViewInterface?
    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        ViewInterface::DrawInputString(draw_str);
    }

    if (manager.HasNewMessages() || redraw_messages)
    {
        // TODO draw arrows and the ability to scroll messages.
        // I have a feeling this is gonna be slow as ever.

        DrawMessages();

        redraw_messages = false;
    }
}

void ChatView::HandleInput()
{
    // Check if this is a command
    if (usr_input[0] == '/')
    {
        ChangeView(usr_input);
    }
    else
    {
        // TODO switch to using C strings so we don;t need to extend
        // strings for each added character
        // and then we can just memcpy?
        const String& username = *setting_manager.Username();
        String msg = "00:00 ";
        msg += username;
        msg += ": ";
        msg += usr_input;

        // prepare ascii message, encode into Message + Packet
        qchat::Ascii ascii(
            manager.ActiveRoom()->room_uri,
            {msg.c_str()}
        );

        // TODO move into encode...
        // TODO packet should maybe have a static next_packet_id?
        std::unique_ptr<SerialPacket> packet = std::make_unique<SerialPacket>(HAL_GetTick(), 1);
        packet->SetData(Packet::Types::Message, 0, 1);
        packet->SetData(manager.NextPacketId(), 1, 2);

        // The packet length is set in the encode function
        // TODO encode probably could just generate a packet instead...
        qchat::Codec::encode(packet, 3, ascii);

        uint64_t new_offset = packet->NumBytes();
        // Expiry time
        packet->SetData(0xFFFFFFFF, new_offset, 4);
        new_offset += 4;
        // Creation time
        packet->SetData(0, new_offset, 4);
        new_offset += 4;

        // TODO ENABLE
        manager.EnqueuePacket(std::move(packet));
        manager.PushMessage(std::move(msg));

        // redraw_messages = true;

        msg_id++;

    }
}


void ChatView::DrawMessages()
{
    const Vector<String>& messages = manager.GetMessages();

    int32_t msg_idx = messages.size() - 1;

    // If there are no messages just return
    if (msg_idx < 0) return;

    uint16_t y_window_start = (menu_font.height + Padding_3);
    uint16_t y_window_end = screen.ViewHeight() - (usr_font.height + usr_font.height / 2);
    uint16_t y_window_size = y_window_end - y_window_start;

    uint16_t x_window_start = Padding_2;
    uint16_t x_window_end = screen.ViewWidth() - Padding_2;
    uint16_t x_window_size = x_window_end - x_window_start;

    uint16_t total_used_y_space = 0;
    uint16_t curr_used_y_space = 0;
    uint16_t prev_used_y_space = 0;

    uint16_t x_pos = x_window_start;
    uint16_t curr_used_x_space = x_pos;
    uint16_t next_used_x_space = 0;

    // Get the msg size of the first message in the list
    curr_used_x_space = x_window_start + messages[msg_idx].length() * usr_font.width;

    // clip to screen width
    if (curr_used_x_space > screen.ViewWidth())
        curr_used_x_space = screen.ViewWidth();

    uint16_t y_pos;
    do
    {
        // The x_pos always starts a little offset
        x_pos = x_window_start;

        // Get the next message
        auto& msg = messages[msg_idx];

        // Swap the current used space and previous
        prev_used_y_space = curr_used_y_space;

        // Vertical space used to write the message
        curr_used_y_space = usr_font.height * (1 +
            (curr_used_x_space / x_window_size));

        // Sum the total spaced used
        total_used_y_space += curr_used_y_space;

        // Y position is calculated in reverse, so the total space we have
        // minus how much used so far
        y_pos = y_window_end - total_used_y_space;

        // Only calculate the next x space used on not last msgs
        if (msg_idx > 0)
        {
            // Calculate the amount of space based on the number of characters
            // used
            next_used_x_space = x_pos + (messages[msg_idx - 1].length()) * usr_font.width;

            if (next_used_x_space > screen.ViewWidth())
                next_used_x_space = screen.ViewWidth();
        }

        // Check to see if we need to clear characters
        // Clear the line.

        // TODO only clear the amount of space from the last line as needed
        // We can calculate that by taking the amount of x length and
        // modulus divide by the total window x space
        if (next_used_x_space > curr_used_x_space ||
            curr_used_y_space > usr_font.height)
        {
            uint16_t clipped_clear_box_y = y_pos;
            if (clipped_clear_box_y < y_window_start)
                clipped_clear_box_y = y_window_start;

            uint16_t clear_rec_x_start = curr_used_x_space;
            if (curr_used_y_space > usr_font.height)
                clear_rec_x_start = x_window_start;

            // We have a special case for the first box clear
            if (msg_idx == static_cast<int32_t>(messages.size()) - 1)
            {
                // Clear from the expected y position
                screen.FillRectangle(clear_rec_x_start,
                    clipped_clear_box_y,
                    next_used_x_space, y_pos + curr_used_y_space, bg);
            }
            else
            {
                screen.FillRectangle(clear_rec_x_start,
                    clipped_clear_box_y + (curr_used_y_space - prev_used_y_space),
                    next_used_x_space, y_pos + curr_used_y_space, bg);
            }
        }

        // Swap the curr x space with the next x space;
        curr_used_x_space = next_used_x_space;

        // Draw the body
        screen.DrawTextbox(x_pos, y_pos, x_window_start, y_window_start,
            x_window_end, y_window_end, msg.c_str(), usr_font,
            settings.body_colour, bg);

        // Decrement the idx
        msg_idx--;
    } while (total_used_y_space < y_window_size && msg_idx >= 0);
}

void ChatView::DrawTitle()
{
    // Clear the title space
    screen.FillRectangle(Margin_0, Margin_0, screen.ViewWidth(), menu_font.height + Padding_3, bg);

    // Draw horizontal seperator
    screen.FillRectangle(Margin_0, menu_font.height + Padding_2, screen.ViewWidth(), menu_font.height + Padding_3, fg);

    // Draw team chat name
    screen.DrawText(Margin_0, Padding_2, "@CTO Team - Chat", menu_font, fg, bg);
}

void ChatView::DrawUsrInputSeperator()
{
    // Draw typing seperator
    screen.FillRectangle(Margin_0, screen.ViewHeight() - usr_font.height - Padding_3, screen.ViewWidth(), screen.ViewHeight() - usr_font.height - Padding_2, fg);
}
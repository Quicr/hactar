#include "ChatView.hh"
#include "UserInterfaceManager.hh"
#include "TeamView.hh"
#include "SettingsView.hh"

ChatView::ChatView(UserInterfaceManager& manager,
                   Screen& screen,
                   Q10Keyboard& keyboard,
                   SettingManager& settings)
    : ViewBase(manager, screen, keyboard, settings)
{
    // messages = new Vector<String>();
    // for (int i = 0; i < 15; i++)
    // {
    //     Message msg("00:00", "George", "Hello1 hello2 hello3 hello4"); //hello5 hello6 hello7 hello8 hello9 hello10 hello11 hello12 hello13 hello14 hello15");
    //     messages.push_back(msg);
    // }
}

ChatView::~ChatView()
{

}

void ChatView::Get()
{
    if (!manager.HasMessages()) return;

    Vector<Message>& msgs = manager.GetMessages();

    // TODO std::move
    // TODO note- will this cause issues with references being deleted
    // with the stack?
    for (uint16_t i = 0; i < msgs.size(); i++)
    {
        messages.push_back(msgs[i]);
    }

    manager.ClearMessages();

    redraw_messages = true;
}

void ChatView::AnimatedDraw()
{

}

void ChatView::Draw()
{
    ViewBase::Draw();

    if (redraw_menu)
    {
        if (first_load)
        {
            DrawUsrInputSeperator();
            DrawTitle();
            first_load = false;
        }
    }

    if (usr_input.length() > last_drawn_idx || redraw_input)
    {
        // Shift over and draw the input that is currently in the buffer
        String draw_str;
        draw_str = usr_input.substring(last_drawn_idx);
        last_drawn_idx = usr_input.length();
        DrawInputString(draw_str);
    }

    if (redraw_messages)
    {
        // TODO draw arrows and the ability to scroll messages.
        // I have a feeling this is gonna be slow as ever.

        DrawMessages();

        redraw_messages = false;
    }
}

bool ChatView::HandleInput()
{
    // Handle the input from the user
    // If they enter a command for going to settings then change views
    GetInput();

    if (!keyboard.EnterPressed()) return false;

    // Parse commands
    if (!(usr_input.length() > 0)) return false;

    // Check if this is a command
    if (usr_input[0] == '/')
    {
        String command = usr_input.substring(1);

        // TODO put a command "flag match" into each .hh file so that it can
        // be registered instead of hardcoded.


        // Is there a better way of doing this?
        if (command == "t")
        {
            manager.ChangeView<TeamView>();
            return true;
        }
        else if (command == "s")
        {
            manager.ChangeView<SettingsView>();
            return true;
        }
    }
    else
    {
        Message msg;

        // TODO get from a RTC
        msg.Timestamp("00:00");

        // TODO get from EEPROM
        msg.Sender("Brett");
        msg.Body(usr_input);

        messages.push_back(msg);

        Packet packet(HAL_GetTick(), 1);

        // Set the type
        packet.SetData(Packet::Types::Message, 0, 6);

        // Set the id
        packet.SetData(manager.NextPacketId(), 6, 8);

        // Set the data length
        packet.SetData(msg.Length(), 14, 10);

        // Append the data
        // TODO update the packet arr to take unsigned char and signed char
        packet.SetDataArray(reinterpret_cast<const unsigned char*>(
            msg.Concatenate().c_str()), msg.Length(), 24);

        manager.EnqueuePacket(std::move(packet));

        redraw_messages = true;


        // Send message to sec layer
        // TODO
    }

    ClearInput();
    return false;

}


void ChatView::DrawMessages()
{
    // Need -1 for the conditional loop
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
    curr_used_x_space = x_window_start + (messages[msg_idx].Length() +
        name_seperator.length()) * usr_font.width;

    // clip to screen width
    if (curr_used_x_space > screen.ViewWidth())
        curr_used_x_space = screen.ViewWidth();

    uint16_t y_pos;
    do
    {
        // The x_pos always starts a little offset
        x_pos = x_window_start;

        // Get the next message
        Message& msg = messages[msg_idx];

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
            next_used_x_space = x_pos + (messages[msg_idx-1].Length() +
                name_seperator.length()) * usr_font.width;

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

        // Draw the timestamp
        screen.DrawTextbox(x_pos, y_pos, x_window_start, y_window_start,
            x_window_end, y_window_end, msg.Timestamp(), usr_font,
            settings.timestamp_colour, bg);
        x_pos += usr_font.width * msg.Timestamp().length();

        // Draw the sender
        screen.DrawTextbox(x_pos, y_pos, x_window_start, y_window_start,
            x_window_end, y_window_end, msg.Sender(), usr_font,
            settings.name_colour, bg);
        x_pos += usr_font.width * msg.Sender().length();

        // Draw the seperator
        screen.DrawTextbox(x_pos, y_pos, x_window_start, y_window_start,
            x_window_end, y_window_end, ": ", usr_font, fg, bg);
        x_pos += usr_font.width * 2;

        // Draw the body
        screen.DrawTextbox(x_pos, y_pos, x_window_start, y_window_start,
            x_window_end, y_window_end, msg.Body(), usr_font,
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
#include "keyboard_display.hh"
#include "main.h"
#include "ui_net_link.hh"
#include <cstring>

void InitScreen(Screen& screen)
{
    screen.Init();
    // Do the first draw
    screen.FillRectangle(0, WIDTH, 0, HEIGHT, Colour::Black);
    for (int i = 0; i < 320; i += Screen::Num_Rows)
    {
        screen.Draw(0xFFFF);
    }
    screen.EnableBacklight();
}

void HandleChatMessages(Screen& screen, link_packet_t* packet)
{
    UI_LOG_INFO("Got text message");

    // Ignore the first bytes, which are just header information
    // +1 channel id, +1 message type, +4 length
    constexpr uint8_t payload_offset = 6;
    char* text = (char*)(packet->payload + payload_offset);

    screen.CommitText(text, packet->length - payload_offset);
}

void HandleKeypress(Screen& screen, Keyboard& keyboard, Serial& serial, Protector& protector)
{
    while (keyboard.NumAvailable() > 0)
    {
        const uint8_t ch = keyboard.Read();
        link_packet_t packet;

        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_RESET);

        switch (ch)
        {
        case Keyboard::Ent:
        {
            ui_net_link::Serialize(ui_net_link::Channel_Id::Chat, screen.UserText(),
                                   screen.UserTextLength(), packet);

            if (!protector.TryProtect(&packet))
            {
                UI_LOG_ERROR("Failed to encrypt text packet");
                return;
            }

            UI_LOG_INFO("Transmit text message");
            serial.Write(packet);

            screen.CommitText(screen.UserText(), screen.UserTextLength());
            screen.ClearUserText();
            break;
        }
        case Keyboard::Bak:
        {
            screen.BackspaceUserText();
            break;
        }
        default:
        {
            screen.AppendUserText(ch);
            break;
        }
        }
    }
}

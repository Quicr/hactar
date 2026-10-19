#include "keyboard_display.hh"
#include "graphics.hh"
#include "main.h"
#include "ui_net_link.hh"
#include <cstdint>
#include <cstring>

void InitScreen(Screen& screen)
{
    screen.Init();
    screen.FillScreen(Graphics::Colour::Black);
    screen.Draw(0);
    screen.EnableBacklight();
}

void HandleChatMessages(Screen& screen, link_packet_t* packet)
{

    // Ignore the first bytes, which are just header information
    // +1 channel id, +1 message type, +4 length
    constexpr uint8_t payload_offset = 6;
    char* text = (char*)(packet->payload.data() + payload_offset);

    UI_LOG_INFO("Got text message of len %d", packet->length - payload_offset);
    screen.CommitText(text, packet->length - payload_offset);
}

void HandleKeypress(Screen& screen, Keyboard& keyboard, Serial& serial, Protector& protector)
{
    while (keyboard.NumAvailable())
    {
        const uint8_t ch = keyboard.Read();

        HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_RESET);

        switch (ch)
        {
        case Keyboard::Ent:
        {
            if (screen.UserTextLength() == 0)
            {
                return;
            }

            link_packet_t packet;
            uint32_t offset = 0;
            const uint32_t text_len = screen.UserTextLength();

            packet.type = static_cast<uint16_t>(ui_net_link::UiToNet::AudioFrame);
            packet.payload[offset] = static_cast<uint8_t>(ui_net_link::Channel_Id::Chat);
            offset += sizeof(uint8_t);

            packet.payload[offset] = static_cast<uint8_t>(ui_net_link::MessageType::Chat);
            offset += sizeof(uint8_t);

            memcpy(packet.payload.data() + offset, &text_len, sizeof(uint32_t));
            offset += sizeof(uint32_t);

            packet.length = offset + text_len;

            memcpy(packet.payload.data() + offset, screen.UserText(), text_len);

            if (!protector.TryProtect(&packet))
            {
                UI_LOG_ERROR("Failed to encrypt text packet");
                return;
            }

            UI_LOG_INFO("Transmit text message");
            serial.Write(packet);

            screen.CommitText(screen.UserText(), text_len);
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

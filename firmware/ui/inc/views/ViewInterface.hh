#pragma once

#include <functional>

#include "UserInterfaceManager.hh"
#include "Screen.hh"
#include "Q10Keyboard.hh"
#include "EEPROM.hh"
#include "CommandHandler.hh"
#include "SerialPacketManager.hh"
#include "network.hh"
#include "audio_chip.hh"
#include <string>


class UserInterfaceManager;

class ViewInterface
{
public:
    ViewInterface(UserInterfaceManager& manager,
        Screen& screen,
        Q10Keyboard& keyboard,
        SettingManager& setting_manager,
        SerialPacketManager& serial,
        Network& network,
        AudioChip& audio
    ):
        manager(manager),
        screen(screen),
        keyboard(keyboard),
        setting_manager(setting_manager),
        serial(serial),
        network(network),
        audio(audio),
        command_handler(&manager),
        first_load(true),
        redraw_menu(true),
        cursor_animate_timeout(0),
        redraw_cursor(true),
        cursor_fill(true),
        backspace_count(0),
        last_drawn_idx(0),
        redraw_input(true),
        usr_input(""),
        tx_colour(0),
        rx_colour(0),
        tx_redraw_timeout(0),
        rx_redraw_timeout(0),
        general_refresh_timeout(0),
        mic_last_state(true)
    {
        // TODO load settings from eeprom


        ResetCursorPosition();
        Clear();
    }

    virtual ~ViewInterface()
    {
    }

    virtual void Run()
    {
        AnimatedDraw();
        Draw();
        BaseUpdate();
    }

    void Clear()
    {
        screen.DisableBackLight();
        screen.FillScreen(bg);
        screen.EnableBackLight();
    }
protected:
    // Cursor types and const expr
    typedef struct
    {
        uint16_t x = 0;
        uint16_t y = 0;
    } cursor_position_t;
    static constexpr uint16_t Cursor_Animate_Duration = 2500;
    static constexpr uint16_t Cursor_Hollow_Thickness = 1;

    // TODO add current tick
    bool BaseUpdate()
    {
        // Handle input updates
        InputUpdate();

        // Run defined view Update
        Update();

        // Change view if set
        if (command_handler.ChangeViewCommand(new_view))
        {
            return true;
        }

        // Clear the new_view string if it is not a view it should be blank
        new_view.clear();
        return false;
    }

    // TODO put most of input base code here
    void InputUpdate()
    {
        GetInput();

        if (!keyboard.EnterPressed()) return;
        if (!(usr_input.length() > 0)) return;

        // Actual view code goes here
        HandleInput();

        ClearInput();
    }

    virtual void Update() = 0;
    virtual void HandleInput() = 0;
    virtual void AnimatedDraw() = 0;
    virtual void Draw()
    {
        DrawInput();

        // Draw Tx and Rx

        if (tx_colour != TxStatusColour() &&
            HAL_GetTick() > tx_redraw_timeout)
        {
            tx_colour = TxStatusColour();
            screen.FillArrow(screen.ViewWidth() - 12, 0, 10, 4, Screen::ArrowDirection::Up, tx_colour);
            tx_redraw_timeout = HAL_GetTick() + 200;
        }

        if (rx_colour != RxStatusColour() &&
            HAL_GetTick() > rx_redraw_timeout)
        {
            rx_colour = RxStatusColour();
            screen.FillArrow(screen.ViewWidth() - 6, 10, 10, 4, Screen::ArrowDirection::Down, rx_colour);
            rx_redraw_timeout = HAL_GetTick() + 200;
        }

        if (HAL_GetTick() > general_refresh_timeout)
        {
            DrawWifiSymbol();
        }

        if (mic_last_state != keyboard.MicPressed())
        {
            mic_last_state = keyboard.MicPressed();

            // Draw the symbol
            uint16_t colour = C_WHITE;
            if (mic_last_state)
            {
                colour = C_GREEN;
            }

            screen.FillCircle(screen.ViewWidth() - 48, 4, 3, colour, C_BLACK);
            screen.FillCircle(screen.ViewWidth() - 48, 8, 3, colour, C_BLACK);
            screen.FillRectangle(screen.ViewWidth() - 51, 4, screen.ViewWidth() - 44, 8, colour);
        }
    }

    virtual void DrawInput()
    {
        DrawBackspace();
        DrawCursor();
    }

    virtual void DrawCursor()
    {
        if (HAL_GetTick() > cursor_animate_timeout || redraw_cursor)
        {
            // Set the next timeout
            cursor_animate_timeout = HAL_GetTick() +
                Cursor_Animate_Duration;

            if (cursor_fill)
            {
                // Draw a filled in rectangle
                screen.FillRectangle(cursor_pos.x,
                    cursor_pos.y,
                    cursor_pos.x + usr_font.width,
                    cursor_pos.y + usr_font.height,
                    fg);
            }
            else
            {
                // Clear the cursor
                screen.FillRectangle(cursor_pos.x,
                    cursor_pos.y,
                    cursor_pos.x + usr_font.width,
                    cursor_pos.y + usr_font.height,
                    bg);

                // Draw a hollow rectangle
                screen.DrawRectangle(cursor_pos.x,
                    cursor_pos.y,
                    cursor_pos.x + usr_font.width,
                    cursor_pos.y + usr_font.height,
                    Cursor_Hollow_Thickness,
                    fg);
            }

            // Swap the cursor fill flag
            cursor_fill = !cursor_fill;
            redraw_cursor = false;
        }
    }

    // Non-virtual functions
    void DrawBackspace()
    {
        if (backspace_count)
        {
            // Clear from the end to the front
            uint32_t clear_width = backspace_count * usr_font.width;
            cursor_pos.x -= clear_width;
            screen.FillRectangle(cursor_pos.x,
                cursor_pos.y,
                cursor_pos.x + clear_width + usr_font.width,
                cursor_pos.y + usr_font.height,
                bg);
            backspace_count = 0;
            last_drawn_idx = usr_input.length();
            redraw_cursor = true;
            cursor_fill = true;
        }
    }

    void DrawInputString(const std::string& str)
    {
        last_drawn_idx = usr_input.length();
        screen.DrawText(cursor_pos.x, cursor_pos.y, str,
            usr_font, fg, bg);
        // Set the cursor the the length of the input
        cursor_pos.x = usr_font.width * usr_input.length();
        redraw_cursor = true;
        cursor_fill = true;
        redraw_input = false;
    }

    void GetInput()
    {
        // Get input
        redraw_input = keyboard.Read(usr_input);

        // If the back button is pressed anda there is characters in the buffer
        if (keyboard.BackPressed() && usr_input.length())
        {
            usr_input.pop_back();
            redraw_cursor = true;
            backspace_count += 1;
        }
    }

    void ClearInput()
    {
        // Clear the characters and previous cursor
        backspace_count = usr_input.length();

        // Clear our input buffer
        usr_input.clear();

        // Draw the input
        DrawInput();
    }

    void ChangeView(const char* str)
    {
        new_view = str;
    }

    void ChangeView(const std::string& str)
    {
        new_view = str;
    }

    void ResetCursorPosition()
    {
        SetCursorX(0);
        SetCursorY(screen.ViewHeight() - usr_font.height);
    }

    void SetCursorX(uint16_t x)
    {
        // TODO clipping
        cursor_pos.x = x;
    }

    void SetCursorY(uint16_t y)
    {
        // TODO clipping
        cursor_pos.y = y;
    }

    UserInterfaceManager& manager;
    Screen& screen;
    Q10Keyboard& keyboard;
    SettingManager& setting_manager;
    SerialPacketManager& serial;
    Network& network;
    AudioChip& audio;
    CommandHandler command_handler;

    // If this is the first load, then we should
    // Run the first load draw
    bool first_load;

    // Menu drawing variables
    bool redraw_menu;

    // Cursor draw variables
    cursor_position_t cursor_pos;
    uint32_t cursor_animate_timeout;
    bool redraw_cursor;
    bool cursor_fill;

    // Input draw variables
    uint16_t backspace_count;
    size_t last_drawn_idx;
    bool redraw_input;

    // Input variables
    std::string usr_input;

    uint32_t tx_colour;
    uint32_t rx_colour;

    uint32_t tx_redraw_timeout;
    uint32_t rx_redraw_timeout;

    uint32_t general_refresh_timeout;

    bool mic_last_state;

    // TODO EEPROM setting
    Font& menu_font = font11x16;
    // TODO EEPROM setting
    Font& usr_font = font7x12;
    // TODO EERPOM setting
    uint16_t fg = C_WHITE;
    // TODO EEPROM setting
    uint16_t bg = C_BLACK;
private:
    std::string new_view;

    inline uint32_t TxStatusColour() const
    {
        return ConvertSerialStatusToColour(serial.GetTxStatus());
    }

    inline uint32_t RxStatusColour() const
    {
        return ConvertSerialStatusToColour(serial.GetRxStatus());
    }

    uint32_t ConvertSerialStatusToColour(
        const SerialPacketManager::SerialStatus status) const
    {
        if (status == SerialPacketManager::SerialStatus::OK)
            return C_GREEN;
        else if (status == SerialPacketManager::SerialStatus::PARTIAL)
            return C_CYAN;
        else if (status == SerialPacketManager::SerialStatus::TIMEOUT)
            return C_MAGENTA;
        else if (status == SerialPacketManager::SerialStatus::BUSY)
            return C_YELLOW;
        else if (status == SerialPacketManager::SerialStatus::ERROR)
            return C_RED;
        else if (status == SerialPacketManager::SerialStatus::CRITICAL_ERROR)
            return C_BLUE;
        else
            return C_WHITE;
    }

    void DrawWifiSymbol()
    {
        uint16_t colour = C_GREY;
        if (network.IsConnected())
        {
            colour = C_LIGHT_GREEN;
        }

        // Draw wifi symbol
        const uint16_t y_start_offset = 3;
        screen.FillRectangle(screen.ViewWidth() - 30, y_start_offset + 0, screen.ViewWidth() - 20, y_start_offset + 1, colour);
        screen.FillRectangle(screen.ViewWidth() - 28, y_start_offset + 2, screen.ViewWidth() - 22, y_start_offset + 3, colour);
        screen.FillRectangle(screen.ViewWidth() - 26, y_start_offset + 4, screen.ViewWidth() - 24, y_start_offset + 5, colour);
    }
};
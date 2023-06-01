#pragma once

#include <functional>

#include "UserInterfaceManager.hh"
#include "Screen.hh"
#include "Q10Keyboard.hh"
#include "EEPROM.hh"
#include "CommandHandler.hh"
#include "String.hh"


class UserInterfaceManager;

class ViewBase
{
public:
    ViewBase(UserInterfaceManager &manager,
             Screen &screen,
             Q10Keyboard &keyboard,
             SettingManager& setting_manager):
        manager(manager),
        screen(screen),
        keyboard(keyboard),
        setting_manager(setting_manager),
        command_handler(new CommandHandler(&manager)),
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
        rx_redraw_timeout(0)
    {
        ResetCursorPosition();
        Clear();
    }

    virtual ~ViewBase()
    {
    }

    virtual void Run()
    {
        if (Update()) return; // TODO somehow need to return here so change views doesn't crash
        if (ScanInput()) return;
        AnimatedDraw();
        Draw();
    }

    void Clear()
    {
        screen.DisableBackLight();
        screen.FillScreen(bg);
        screen.EnableBackLight();
    }
protected:
    // Cursor types and const expr
    typedef struct {
        uint16_t x = 0;
        uint16_t y = 0;
    } cursor_position_t;
    static constexpr uint16_t Cursor_Animate_Duration = 2500;
    static constexpr uint16_t Cursor_Hollow_Thickness = 1;

    // TODO define some update function
    virtual bool Update() = 0;
     // TODO put most of input base code here
     // Need to think of a different way of changing views. Probably should be
     // In update or something
    virtual bool ScanInput()
    {
        GetInput();


        if (!keyboard.EnterPressed()) return false;
        if (!(usr_input.length() > 0)) return false;

        // Acutal view code goes here
        if (HandleInput()) return true;

        ClearInput();

        return false;
    }

    virtual bool HandleInput() = 0;
    virtual void AnimatedDraw() = 0;
    virtual void Draw()
    {
        DrawInput();

        // Draw Tx and Rx
        if (tx_colour != manager.GetTxStatusColour() &&
            HAL_GetTick() > tx_redraw_timeout)
        {
            tx_colour = manager.GetTxStatusColour();
            screen.DrawText(WIDTH-32, 0, "tx", font5x8, tx_colour, bg);
            tx_redraw_timeout = HAL_GetTick() + 200;
        }

        if (rx_colour != manager.GetRxStatusColour() &&
            HAL_GetTick() > rx_redraw_timeout)
        {
            rx_colour = manager.GetRxStatusColour();
            screen.DrawText(WIDTH-16, 0, "rx", font5x8, rx_colour, bg);
            rx_redraw_timeout = HAL_GetTick() + 200;
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

    void DrawInputString(const String &str)
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

    UserInterfaceManager &manager;
    Screen &screen;
    Q10Keyboard &keyboard;
    SettingManager& setting_manager;
    CommandHandler* command_handler;

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
    String usr_input;

    uint32_t tx_colour;
    uint32_t rx_colour;

    uint32_t tx_redraw_timeout;
    uint32_t rx_redraw_timeout;

    // TODO EEPROM setting
    Font &menu_font = font11x16;
    // TODO EEPROM setting
    Font &usr_font = font7x12;
    // TODO EERPOM setting
    uint16_t fg = C_WHITE;
    // TODO EEPROM setting
    uint16_t bg = C_BLACK;
};
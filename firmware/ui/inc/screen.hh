#pragma once

#include "app_main.hh"
#include "font.hh"
#include "graphics.hh"
#include "ili9341.hh"
#include "stm32.h"
#include "ui_net_link.hh"
#include <cstdint>
#include <type_traits>

class Screen
{
public:
    static constexpr uint32_t Num_Rows = 10;
    static constexpr uint16_t Title_y1 = 2;
    static constexpr uint32_t Title_Length = 21;
    static constexpr uint32_t Max_Texts = 35;
    static constexpr uint32_t Max_Characters = 48;
    static constexpr uint16_t Top_Fixed_Height = 20;
    static constexpr uint16_t Bottom_Fixed_Height = 20;

    // TODO calculate

private:
    struct YBound
    {
        uint16_t y1 = 0;
        uint16_t y2 = 0;
    };

    enum class MemoryStatus : uint8_t
    {
        Free = 0,
        In_Progress,
    };

    // Some basic colours (RGB565)
    static constexpr uint16_t C_BLACK = 0x0000U;
    static constexpr uint16_t C_WHITE = 0xFFFFU;
    static constexpr uint16_t C_BLUE = 0x001FU;
    static constexpr uint16_t C_RED = 0xF800U;
    static constexpr uint16_t C_LIGHT_GREEN = 0x3626U;
    static constexpr uint16_t C_GREEN = 0x07E0U;
    static constexpr uint16_t C_CYAN = 0x07FFU;
    static constexpr uint16_t C_MAGENTA = 0xF81FU;
    static constexpr uint16_t C_YELLOW = 0xFFE0U;
    static constexpr uint16_t C_GREY = 0xCE59U;
    static constexpr uint16_t Colour_Map[]{C_BLACK, C_WHITE, C_BLUE,    C_RED,    C_LIGHT_GREEN,
                                           C_GREEN, C_CYAN,  C_MAGENTA, C_YELLOW, C_GREY};

public:
    Screen(uint8_t* scanline_buffer,
           const uint16_t num_lines,
           const uint16_t total_size,
           uint8_t* data_buff,
           const uint16_t width,
           const uint16_t height,
           SPI_HandleTypeDef& hspi,
           GPIO_TypeDef* cs_port,
           const uint16_t cs_pin,
           GPIO_TypeDef* dc_port,
           const uint16_t dc_pin,
           GPIO_TypeDef* rst_port,
           const uint16_t rst_pin,
           GPIO_TypeDef* bl_port,
           const uint16_t bl_pin,
           ILI9341::Orientation orientation);

    void Draw(uint32_t timeout);
    void Reset();
    void Sleep();
    void Wake();
    void EnableBacklight();
    void DisableBacklight();
    inline void Select();
    inline void Deselect();

    void ScrollScreen(const uint16_t scroll_idx, bool up);

    void FillScreen(const Graphics::Colour colour);
    void UpdateTitle(const char* title, const uint32_t len);

    void AppendText(const char* text, const uint32_t len);
    void CommitText();
    void CommitText(const char* text, const uint32_t len);

    void AppendUserText(const char* text, const uint32_t len);
    void AppendUserText(const char ch);

    void BackspaceUserText();
    void ClearUserText();

    const char* UserText() const noexcept;
    uint16_t UserTextLength() const noexcept;

private:
    enum FlagType : uint32_t
    {
        Title_Dirty = 1 << 0,
        Icons_Dirty = 1 << 1,
        Text_Area_Dirty = 1 << 2,
        Usr_Text_Dirty = 1 << 3,
    };

    void RotateScreen(ILI9341::Orientation orientation);
    void CalculateDrawAreas();
    void RaiseFlag(const FlagType flag);
    void LowerFlag(const FlagType flag);

    // Private functions
    const Font& title_font = font11x16;
    const Font& text_font = font5x8;

    // Variables
    uint8_t* scanline_buffer;
    const uint16_t num_lines;
    const uint16_t total_size;

    ILI9341 ili9341;

    uint16_t row;
    uint16_t end_row;

    // Window: 0-20px
    // Max 21 characters
    char title_buffer[Title_Length];

    // Window: 20 - 308px
    uint32_t text_idx;
    uint32_t texts_in_use;
    uint32_t scroll_offset;
    char text_buffer[Max_Texts][Max_Characters];
    char text_lens[Max_Texts];

    // Window: 308-320px
    char usr_buffer[Max_Characters];
    char usr_buffer_idx;

    uint16_t title_x1;
    uint16_t title_x2;
    uint16_t icons_x_offset;
    uint16_t scroll_area_height;
    uint16_t usr_text_y_offset;

    uint32_t flags;
};

// 44226 bytes approximately.

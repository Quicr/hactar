#pragma once

#include "app_main.hh"
#include "font.hh"
#include "graphics.hh"
#include "gsl/gsl-lite.hpp"
#include "ili9341.hh"
#include "ring_buffer.hh"
#include "stm32.h"
#include "ui_net_link.hh"
#include <cstdint>
#include <type_traits>

class Screen
{
public:
    // TODO revist these offsets
    static constexpr uint16_t Title_y1 = 2;
    static constexpr uint32_t Max_Title_Length = 21;
    static constexpr uint32_t Max_Characters = 320 / 5;
    static constexpr uint16_t Top_Fixed_Height = 20;
    static constexpr uint16_t Bottom_Fixed_Height = 12;
    static constexpr uint32_t Max_Texts = (240 - Top_Fixed_Height - Bottom_Fixed_Height) / 8;

private:
    enum class MemoryStatus : uint8_t
    {
        Free = 0,
        In_Progress,
    };

    // Some basic colours (RGB565)
    // should this be in ili9341?
    static constexpr uint16_t C_BLACK = __builtin_bswap16(0x0000U);
    static constexpr uint16_t C_WHITE = __builtin_bswap16(0xFFFFU);
    static constexpr uint16_t C_BLUE = __builtin_bswap16(0x001FU);
    static constexpr uint16_t C_RED = __builtin_bswap16(0xF800U);
    static constexpr uint16_t C_LIGHT_GREEN = __builtin_bswap16(0x3626U);
    static constexpr uint16_t C_GREEN = __builtin_bswap16(0x07E0U);
    static constexpr uint16_t C_CYAN = __builtin_bswap16(0x07FFU);
    static constexpr uint16_t C_MAGENTA = __builtin_bswap16(0xF81FU);
    static constexpr uint16_t C_YELLOW = __builtin_bswap16(0xFFE0U);
    static constexpr uint16_t C_GREY = __builtin_bswap16(0xCE59U);
    static constexpr uint16_t Colour_Map[]{C_BLACK, C_WHITE, C_BLUE,    C_RED,    C_LIGHT_GREEN,
                                           C_GREEN, C_CYAN,  C_MAGENTA, C_YELLOW, C_GREY};

public:
    Screen(uint16_t* scanline_buffer,
           const uint16_t num_lines,
           const uint16_t num_cols,
           const uint16_t num_buffs,
           uint8_t* data_buff,
           const uint16_t width,
           const uint16_t height,
           const uint16_t quantization,
           const uint16_t screen_width_pixels,
           const uint16_t screen_height_pixels,
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

    void Init();

    void AnimationUpdate(const uint32_t ticks_ms);
    void Draw(const uint32_t ticks_ms);

    void Reset();
    void Sleep();
    void Wake();
    void EnableBacklight();
    void DisableBacklight();
    inline void Select();
    inline void Deselect();

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

    void PushShape(Graphics::Shape shape);

    void DrawTitleLine();
    void DrawUserLine();

private:
    struct bound_t
    {
        uint16_t x1;
        uint16_t y1;
        uint16_t x2;
        uint16_t y2;
    };

    enum RenderFlags : uint32_t
    {
        Rendering = 1 << 0,
        Whole_Screen_Dirty = 1 << 1,
        Title_Dirty = 1 << 2,
        Icons_Dirty = 1 << 3,
        Text_Area_Dirty = 1 << 4,
        Usr_Text_Dirty = 1 << 5,
    };

    enum AnimationFlags : uint32_t
    {
        CursorSolid = 1 << 0
    };

    void RotateScreen(ILI9341::Orientation orientation);
    void CalculateDrawAreas();
    void DefineScrollArea();
    void ScrollText();
    void Render();

    // TODO should make a flag helper class, as we have a lot of flag raising
    // through out the program
    void RaiseFlag(uint32_t& flags, const uint32_t flag);
    void LowerFlag(uint32_t& flags, const uint32_t flag);
    uint32_t ReadFlag(const uint32_t flags, const uint32_t flag);
    uint32_t TakeFlag(uint32_t& flags);

    void DrawCursor(const bool solid);
    void ExpandBound(
        bound_t& bound, const uint16_t x1, const uint16_t y1, const uint16_t x2, const uint16_t y2);
    void ResetBound(bound_t& bound);

    // Private functions
    const Font& title_font = font11x16;
    const Font& text_font = font5x8;

    // Variables
    uint16_t* scanline_buffer;
    const uint16_t num_lines;
    const uint16_t num_cols;
    const uint16_t num_buffs;

    uint8_t* data_buff;
    const uint16_t data_buff_width;
    const uint16_t data_buff_height;
    const uint16_t quantization;

    ILI9341 ili9341;

    uint16_t* scanline_ptr;

    bound_t draw_bound;
    bound_t icon_bound;
    bound_t text_bound;
    bound_t usr_bound;

    uint16_t title_x1;
    uint16_t title_x2;

    // Window: 0-20px
    // Max 21 characters
    char title_buffer[Max_Title_Length];
    size_t title_len;

    // Window: 20 - 308px
    uint32_t text_write;
    uint32_t text_read;
    uint32_t texts_in_use;
    uint32_t scroll_offset;

    // Move into a ring buffer
    char text_buffer[Max_Texts][Max_Characters];
    char text_lens[Max_Texts];
    size_t max_texts;

    // Window: 308-320px
    char usr_buffer[Max_Characters];
    size_t usr_buffer_idx;

    uint16_t icons_x_offset;
    uint16_t usr_text_y_offset;
    uint16_t texts_area_y_offset;
    uint16_t scroll_area_height;

    uint32_t last_ticks_ms;

    uint32_t last_animation_ticks_ms;
    uint32_t animation_flags;

    uint32_t render_flags;
};

// 44226 bytes approximately.

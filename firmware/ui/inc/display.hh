#pragma once

#include "graphics/ili9341.hh"
#include <cstddef>
#include <cstdint>

class Display
{
public:
    static constexpr uint16_t Width = 240U;
    static constexpr uint16_t Height = 320U;
    static constexpr uint16_t Title_Height = 20U;
    static constexpr uint16_t Messages_Height = 280U;
    static constexpr uint16_t Entry_Height = 20U;
    static_assert(Title_Height + Messages_Height + Entry_Height == Height);

    explicit Display(ILI9341& ili9341);

    void SetTitle(const char* title, size_t length);
    bool Render();

private:
    static constexpr uint16_t Title_Y = 0U;
    static constexpr uint16_t Messages_Y = Title_Y + Title_Height;
    static constexpr uint16_t Entry_Y = Messages_Y + Messages_Height;
    static constexpr uint16_t Title_X = 5U;
    static constexpr uint16_t Title_Text_Y = 2U;
    static constexpr uint16_t Title_Colour = 0xFFFFU;
    static constexpr uint16_t Background_Colour = 0x0000U;
    static constexpr size_t Max_Title_Length = 21U;
    static constexpr size_t Title_Buffer_Size = Width * Title_Height * 2U;

    void DrawTitle();
    void DrawTitleCharacter(uint16_t x, char character);
    void SetPixel(uint16_t x, uint16_t y, uint16_t colour);

    ILI9341& ili9341;
    char title[Max_Title_Length + 1U];
    size_t title_length;
    bool title_dirty;
    uint8_t render_buffer[Title_Buffer_Size];
};

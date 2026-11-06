#include "display.hh"
#include "font.hh"

Display::Display(ILI9341& ili9341) :
    ili9341(ili9341),
    title{0},
    title_length(0U),
    title_dirty(true),
    render_buffer{0}
{
}

void Display::SetTitle(const char* new_title, size_t length)
{
    if (new_title == nullptr)
    {
        length = 0U;
    }

    if (length > Max_Title_Length)
    {
        length = Max_Title_Length;
    }

    bool changed = length != title_length;
    for (size_t i = 0U; i < length; ++i)
    {
        if (title[i] != new_title[i])
        {
            changed = true;
        }
    }

    if (!changed)
    {
        return;
    }

    for (size_t i = 0U; i < length; ++i)
    {
        title[i] = new_title[i];
    }

    title[length] = '\0';
    title_length = length;
    title_dirty = true;
}

bool Display::Render()
{
    if (ili9341.GetWidth() != Width || ili9341.GetHeight() != Height || !title_dirty
        || ili9341.IsSpiBusy())
    {
        return false;
    }

    DrawTitle();
    ili9341.SetRenderRegion(0U, Title_Y, Width - 1U, Title_Height - 1U);

    if (!ili9341.Render(render_buffer, sizeof(render_buffer)))
    {
        return false;
    }

    title_dirty = false;
    return true;
}

void Display::DrawTitle()
{
    const uint8_t background_high = static_cast<uint8_t>(Background_Colour >> 8U);
    const uint8_t background_low = static_cast<uint8_t>(Background_Colour);

    for (size_t i = 0U; i < Title_Buffer_Size; i += 2U)
    {
        render_buffer[i] = background_high;
        render_buffer[i + 1U] = background_low;
    }

    for (size_t i = 0U; i < title_length; ++i)
    {
        DrawTitleCharacter(Title_X + static_cast<uint16_t>(i * font11x16.width), title[i]);
    }
}

void Display::DrawTitleCharacter(uint16_t x, char character)
{
    uint8_t glyph = static_cast<uint8_t>(character);
    if (glyph < 32U || glyph > 126U)
    {
        glyph = static_cast<uint8_t>('?');
    }

    const uint16_t bytes_per_row = (font11x16.width + 7U) / 8U;
    const size_t bytes_per_character = static_cast<size_t>(bytes_per_row) * font11x16.height;
    const uint8_t* character_data = font11x16.data + ((glyph - 32U) * bytes_per_character);

    for (uint16_t row = 0U; row < font11x16.height; ++row)
    {
        for (uint16_t column = 0U; column < font11x16.width; ++column)
        {
            const uint8_t bits = character_data[row * bytes_per_row + (column / 8U)];
            const uint8_t mask = static_cast<uint8_t>(0x80U >> (column % 8U));
            if ((bits & mask) != 0U)
            {
                SetPixel(x + column, Title_Text_Y + row, Title_Colour);
            }
        }
    }
}

void Display::SetPixel(uint16_t x, uint16_t y, uint16_t colour)
{
    if (x >= Width || y >= Title_Height)
    {
        return;
    }

    const size_t offset = (static_cast<size_t>(y) * Width + x) * 2U;
    render_buffer[offset] = static_cast<uint8_t>(colour >> 8U);
    render_buffer[offset + 1U] = static_cast<uint8_t>(colour);
}

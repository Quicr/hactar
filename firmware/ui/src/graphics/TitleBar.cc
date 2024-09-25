#include "TitleBar.hh"

void DrawTitleBar(std::string title, Font& font, uint16_t fg, uint16_t bg, Screen& screen)
{
    // Clear the title space
    screen.FillRectangleAsync(Margin_0, screen.ViewWidth(), Margin_0, font.height + Padding_3, bg);

    // Draw horizontal seperator
    screen.FillRectangleAsync(Margin_0, screen.ViewWidth(), font.height + Padding_2, font.height + Padding_3, fg);

    // Draw team chat name
    screen.DrawStringAsync(Margin_0, Padding_2, title, font, fg, bg, false);
}
#include "TitleBar.hh"

void DrawTitleBar(std::string title, Font& font, uint16_t fg, uint16_t bg, Screen& screen)
{
    // Clear the title space
    screen.FillRectangle(Margin_0, Margin_0, screen.ViewWidth(), font.height + Padding_3, bg);

    // Draw horizontal seperator
    screen.FillRectangle(Margin_0, font.height + Padding_2, screen.ViewWidth(), font.height + Padding_3, fg);

    // Draw team chat name
    screen.DrawText(Margin_0, Padding_2, title, font, fg, bg);
}
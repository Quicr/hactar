#pragma once

#include <string>
#include "Screen.hh"
#include "Font.hh"

#define Margin_0 0

#define Padding_0 0
#define Padding_1 1
#define Padding_2 2
#define Padding_3 3

void DrawTitleBar(std::string title, Font& font, uint16_t fg, uint16_t bg, Screen& screen);

// TODO
void DrawAnimatedTitleBar();
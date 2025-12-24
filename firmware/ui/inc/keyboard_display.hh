#pragma once

#include "keyboard.hh"
#include "link_packet_t.hh"
#include "screen.hh"
#include "serial.hh"
#include <sframe/sframe.h>

extern "C" {

void InitScreen(Screen& screen);
void HandleKeypress(Screen& screen,
                    Keyboard& keyboard,
                    Serial& serial,
                    sframe::MLSContext& mls_ctx);
void HandleChatMessages(Screen& screen, link_packet_t* packet);
}

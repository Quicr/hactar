#pragma once

#include "keyboard.hh"
#include "link_packet_t.hh"
#include "protector.hh"
#include "screen.hh"
#include "serial.hh"

extern "C" {

void InitScreen(Screen& screen);
void HandleKeypress(Screen& screen, Keyboard& keyboard, Serial& serial, Protector& mls_ctx);
void HandleChatMessages(Screen& screen, link_packet_t* packet);
}

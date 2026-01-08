#pragma once

#include "audio_chip.hh"
#include "config_storage.hh"
#include "protector.hh"
#include "screen.hh"
#include "serial.hh"

extern "C" {
void HandleMgmtLinkPackets(Serial& serial, ConfigStorage& storage);
void HandleNetLinkPackets(Serial& serial, Protector& protector, AudioChip& audio, Screen& screen);
};

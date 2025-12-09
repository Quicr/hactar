#pragma once

#include "audio_chip.hh"
#include "config_storage.hh"
#include "serial.hh"

void HandleMgmtLinkPackets(AudioChip& audio_chip, Serial& serial, ConfigStorage& storage);
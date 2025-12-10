#pragma once

#include "audio_chip.hh"
#include "config_storage.hh"
#include "uart.hh"

void HandleMgmtLinkPackets(AudioChip& audio_chip, Uart& serial, ConfigStorage& storage);
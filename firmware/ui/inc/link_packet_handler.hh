#pragma once

#include "audio_chip.hh"
#include "config_storage.hh"
#include "link_packet_t.hh"
#include "protector.hh"
#include "screen.hh"
#include "serial.hh"

extern "C" {
void HandleMgmtLinkPackets(Serial& serial, ConfigStorage& storage);
void SensorConsumer(link_packet_t& packet);
void HandleNetLinkPackets(Serial& serial, Protector& protector, AudioChip& audio, Screen& screen);
};

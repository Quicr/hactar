#pragma once

#include "audio_chip.hh"
#include "config_storage.hh"
#include "protector.hh"
#include "screen.hh"
#include "serial.hh"
#include "ui_mgmt_link.h"

extern "C" {
void HandleNetLinkPackets(Serial& net_serial,
                          Serial& mgmt_serial,
                          Protector& protector,
                          AudioChip& audio);
void HandleMgmtLinkPackets(Serial& mgmt_serial,
                           Serial& net_serial,
                           ConfigStorage& storage,
                           AudioChip& audio,
                           UiLoopbackMode& loopback,
                           AudioDirectionMode& audio_direction_mode);
};

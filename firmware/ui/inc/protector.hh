#pragma once

#include "config_storage.hh"
#include "link_packet_t.hh"
#include <sframe/sframe.h>

class Protector
{
public:
    Protector(ConfigStorage& storage);
    ~Protector();

    bool TryProtect(link_packet_t* link_packet) noexcept;
    bool TryUnprotect(link_packet_t* link_packet) noexcept;

    bool SaveMLSKey();
    bool LoadMLSKey();

private:
    ConfigStorage& storage;
    sframe::MLSContext mls_ctx;
};

#pragma once
#include "link_packet_t.hh"
#include <sframe/sframe.h>
extern "C" {
bool TryProtect(sframe::MLSContext& mls_ctx, link_packet_t* link_packet);
bool TryUnprotect(sframe::MLSContext& mls_ctx, link_packet_t* link_packet);
}

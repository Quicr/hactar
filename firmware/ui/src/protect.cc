#include "protect.hh"
#include <cstring>

bool TryProtect(sframe::MLSContext& mls_ctx, link_packet_t* packet)
try
{
    uint8_t ct[link_packet_t::Payload_Size];
    auto payload = mls_ctx.protect(
        0, 0, ct, sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});

    std::memcpy(packet->payload + 1, payload.data(), payload.size());
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    return false;
}

bool TryUnprotect(sframe::MLSContext& mls_ctx, link_packet_t* packet)
try
{
    auto payload = mls_ctx.unprotect(
        sframe::output_bytes{packet->payload, link_packet_t::Payload_Size}.subspan(1),
        sframe::input_bytes{packet->payload, packet->length}.subspan(1), {});
    packet->length = payload.size() + 1;
    return true;
}
catch (const std::exception& e)
{
    UI_LOG_ERROR("%s", e.what());
    return false;
}

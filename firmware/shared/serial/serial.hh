#pragma once

#include "../../shared_inc/link_packet_t.hh"

class Serial
{
public:
    virtual link_packet_t* Read() = 0;
    virtual void Write(const uint8_t data, const bool end_frame = true) = 0;
    virtual void Write(const link_packet_t& packet, const bool end_frame = true) = 0;
    virtual void Write(const uint8_t* data, const uint16_t size, const bool end_frame = true) = 0;

protected:
};
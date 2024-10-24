#pragma once


#include <iostream>
#include "audio_codec.hh"

#include <map>

class AudioCodec
{
public:
    AudioCodec() = delete;
    ~AudioCodec() = delete;

    static void ALawCompand(const uint16_t* input, uint8_t* output, const size_t len);
    static void ALawExpand(const uint8_t* input, uint16_t* output, const size_t len);
    static uint8_t ALawCompand(const uint16_t sample);
    static uint16_t ALawExpand(uint8_t sample);
private:
};
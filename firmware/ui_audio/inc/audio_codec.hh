#pragma once


#include <stm32.h>
#include "audio_codec.hh"

class AudioCodec
{
public:
    AudioCodec(){}
    void ALawCompand(const uint16_t* input, uint8_t* output, const size_t len);
    void ALawExpand(const uint8_t* input, uint16_t* output, const size_t len);


    uint8_t ALawCompand(const uint16_t sample);
    uint16_t ALawExpand(uint8_t sample);
private:
};
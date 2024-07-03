#pragma once


#include <iostream>
#include "audio_codec.hh"

#include <map>

class AudioCodec
{
public:
    AudioCodec();

    void ALawEncode(uint16_t* input, uint8_t* output, size_t len);
    void ALawDecode(uint8_t* input, uint16_t* output, size_t len);


    uint8_t ALawCompand(uint16_t sample);
    uint16_t ALawExpand(uint8_t sample);
private:
};
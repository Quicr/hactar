#pragma once

#include "stm32.h"

#include "audio_chip.hh"
#include "audio_codec.hh"

#include <map>

class AudioCodec
{
public:
    AudioCodec(AudioChip& audio);

    bool G711Encode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size);
    bool G711Decode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size);

    bool ALawEncode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size);
    bool ALawDecode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size);


private:
    uint8_t ALawCompend(uint16_t sample);
    uint16_t ALawExpand(uint8_t sample);

    AudioChip& audio;

};
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

    void ComputeALawLookupTable();
    bool ALawEncode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size);
    bool ALawDecode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size);


private:
    static constexpr float A_Val = 87.6;

    AudioChip& audio;
    std::map<uint16_t, uint8_t> a_encode_lut;
    std::map<uint8_t, uint16_t> a_decode_lut;

};
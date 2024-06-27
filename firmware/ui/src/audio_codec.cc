#include "audio_codec.hh"

AudioCodec::AudioCodec(AudioChip& audio):
    audio(audio)
{
    ComputeALawLookupTable();
}

void AudioCodec::ComputeALawLookupTable()
{
    const uint32_t map_level = 256;

    // for (uint32_t i = 0; i < map_level; ++i)
    // {
    //     // Normalize x [0, 1]
    //     float x = i / (map_level - 1);

    //     if (x < (1 / A_Val))
    //     {
    //         a_encode_lut[i]
    //     }
    //     else
    //     {

    //     }
    // }
}

bool AudioCodec::ALawEncode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size)
{
    // for (size_t i = 0; i < in_size; ++i)
    // {
    //     if (input[i])
    // }
}

bool AudioCodec::ALawDecode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size)
{

}

bool AudioCodec::G711Encode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size)
{

}

bool AudioCodec::G711Decode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size)
{

}

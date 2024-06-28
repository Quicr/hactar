#include "audio_codec.hh"

#include <math.h>

// https://www.ti.com/lit/an/spra163a/spra163a.pdf

AudioCodec::AudioCodec(AudioChip& audio):
    audio(audio)
{
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

inline uint8_t AudioCodec::ALawCompend(uint16_t sample)
{
    // Heavily influenced from
    // https://en.wikipedia.org/wiki/G.711
    // https://www.ti.com/lit/an/spra634/spra634.pdf

    int16_t exponent = 0;
    uint16_t mantissa = 0;
    uint16_t sign = 0;

    // Get the sign
    if (sample < 0x8000)
    {
        // Sample is positive
        // put the sign in the 8th bit
        sign = 1 << 7;
    }
    else
    {
        // Sample is negative
        // Get the abs of the val
        sample = 0xFFFF - sample;
    }

    // Clip the sample to 12 bits
    sample &= 0x0FFF;


    uint16_t tmp = sample << 4;
    int i;
    // Find the first bit set in our 12 bits
    for (i = 4; i < 16; ++i)
    {
        if (tmp & 0x8000)
        {
            break;
        }

        // Shift left
        tmp  <<= 1;
    }
    exponent = 11 - i;

    // Get our mantissa (abcd)
    if (sample < 256)
    {
        exponent = 0;
        mantissa = (sample >> 1) & 0x0F;
    }
    else
    {
        mantissa = (sample >> exponent) & 0x0F;
        // Shift the exponent to the front of output since the last 4 bits are
        // for our bit values
        exponent <<= 4;
    }

    // Put together and xor by 0x55;
    return (sign + exponent + mantissa) ^ 0x55;
}

inline uint16_t AudioCodec::ALawExpand(uint8_t sample)
{
    // Heavily influenced from
    // https://en.wikipedia.org/wiki/G.711
    // https://www.ti.com/lit/an/spra634/spra634.pdf

    sample ^= 0xD5;
    // Get the first bit of the sample
    uint16_t sign = (sample & 0x80) << 16;

    // Get bits [6:4]
    uint8_t exponent = (sample & 0x70) >> 4;

    // Gets bits [3:0]
    uint16_t mantissa = (sample & 0x0F) << 1;

    // Some spooky magic addition here that is not explained anywhere
    if (exponent == 0)
    {
        mantissa += 1;
    }
    else
    {
        mantissa += 33;
        mantissa << (exponent - 1);
    }

    return mantissa+sign;
}

bool AudioCodec::G711Encode(uint16_t* input, size_t in_size, uint8_t* output, size_t out_size)
{

}

bool AudioCodec::G711Decode(uint8_t* input, size_t in_size, uint16_t* output, size_t out_size)
{

}

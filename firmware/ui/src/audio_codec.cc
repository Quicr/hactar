 #include "audio_codec.hh"

#include <math.h>

void AudioCodec::ALawCompand(const uint16_t* input, uint8_t* output, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        output[i] = ALawCompand(input[i]);
    }
}

void AudioCodec::ALawExpand(const uint8_t* input, uint16_t* output, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        output[i] = ALawExpand(input[i]);
    }
}

inline uint8_t AudioCodec::ALawCompand(const uint16_t u_sample)
{
    // Heavily influenced from
    // https://en.wikipedia.org/wiki/G.711
    // https://www.ti.com/lit/an/spra634/spra634.pdf

    int16_t sample = u_sample;

    int16_t exponent = 0;
    uint16_t mantissa = 0;
    uint16_t sign = 0;

    // Get the sign
    if (sample >= 0)
    {
        // Sample is positive
        // put the sign in the 8th bit
        sign = 0x80;
    }
    else
    {
        // Sample is negative
        // Get the abs of the val
        sample = -sample;
    }

    // Find the first bit set in our 13 bits
    int i = 0;
    for (; i < 7; ++i)
    {
        if ((sample << i) & 0x4000)
        {
            break;
        }
    }

    exponent = 7 - i;

    // Get our mantissa (abcd)
    if (exponent == 0)
    {
        mantissa = (sample >> 4) & 0x0F;
    }
    else
    {
        mantissa = (sample >> (exponent+3)) & 0x0F;
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

    // Restore the value and invert the sign
    sample ^= 0xD5;

    // Get the first bit of the sample
    const uint16_t sign = (sample & 0x80);

    // Get bits [6:4]
    const uint8_t exponent = (sample & 0x70) >> 4;

    // Gets bits [3:0]
    uint16_t mantissa = (sample & 0x0F) << 1;

    // Some spooky m̸̹̫̅͑́a̷̺̪͑̔g̷̛͈̩̪͋͗ī̴̹c̷̲͔̈̓ ȃ̵̘͙d̶̮͘d̵̮͐͠i̷͇̔t̵̡͌̀ͅì̸̥̊o̸͈̬̾̈́n̵̤̤̈́ here that is not explained anywhere
    if (exponent == 0)
    {
        mantissa = (mantissa + 0x0001) << 2;
    }
    else
    {
        mantissa = (mantissa + 0x0021) << (exponent + 1);
    }

    if (sign)
    {
        return -mantissa;
    }
    else
    {
        return mantissa;
    }
}
#include "audio_codec.hh"

#include <math.h>

// https://www.ti.com/lit/an/spra163a/spra163a.pdf


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

uint8_t AudioCodec::ALawCompand(const uint16_t u_sample)
{
    // Heavily influenced from
    // https://en.wikipedia.org/wiki/G.711
    // https://www.ti.com/lit/an/spra634/spra634.pdf

    int16_t sample = u_sample;

    int16_t exponent = 0;
    uint16_t mantissa = 0;
    uint16_t sign = 0;

    // Get the sign
    if (sample > 0)
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

    // printf("sign = %d\n", sign);
    // printf("sample = %d\n", sample);

    // Clip the sample to 12 bits
    if (sample > 0x0FFF)
    {
        sample = 0x0FFF;
    }

    int i = 0;
    // Find the first bit set in our 12 bits
    for (; i < 7; ++i)
    {
        if ((sample << i) & 0x0800)
        {
            break;
        }
    }
    exponent = 7 - i;
    // printf("i = %d, exponent = %d\n", i, exponent);

    // Get our mantissa (abcd)
    if (exponent == 0)
    {
        mantissa = (sample >> 1) & 0x0F;
    }
    else
    {
        mantissa = (sample >> exponent) & 0x0F;
        // Shift the exponent to the front of output since the last 4 bits are
        // for our bit values
        exponent <<= 4;
    }
    // printf("mantissa = %d\n", mantissa);
    // printf("exponent = %d\n", exponent);
    // printf("together = %d\n", (sign + exponent + mantissa) ^ 0x55);

    // Put together and xor by 0x55;
    return (sign + exponent + mantissa) ^ 0x55;
}

uint16_t AudioCodec::ALawExpand(uint8_t sample)
{
    // Heavily influenced from
    // https://en.wikipedia.org/wiki/G.711
    // https://www.ti.com/lit/an/spra634/spra634.pdf

    // Restore the value and invert the sign
    sample ^= 0xD5;

    // Get the first bit of the sample
    uint16_t sign = (sample & 0x80);

    // Get bits [6:4]
    uint8_t exponent = (sample & 0x70) >> 4;

    // Gets bits [3:0]
    uint16_t mantissa = (sample & 0x0F) << 1;

    // Some spooky m̸̹̫̅͑́a̷̺̪͑̔g̷̛͈̩̪͋͗ī̴̹c̷̲͔̈̓ ȃ̵̘͙d̶̮͘d̵̮͐͠i̷͇̔t̵̡͌̀ͅì̸̥̊o̸͈̬̾̈́n̵̤̤̈́ here that is not explained anywhere
    if (exponent == 0)
    {
        mantissa += 0x0001;
    }
    else
    {
        mantissa = (mantissa + 0x0021) << (exponent - 1);
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
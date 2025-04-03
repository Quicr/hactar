#include "audio_codec.hh"

#include <math.h>

void AudioCodec::ALawCompand(const uint16_t* input, const size_t input_len,
    uint8_t* output, const size_t output_len, const bool input_stereo, const bool output_stereo)
{
    if ((input_stereo && output_stereo) || (!input_stereo && !output_stereo))
    {
        const size_t min_len = input_len < output_len ? input_len : output_len;
        for (size_t i = 0; i < min_len; ++i)
        {
            output[i] = ALawCompand(input[i]);
        }
    }
    else if (input_stereo && !output_stereo)
    {
        uint8_t companded = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < input_len && j < output_len)
        {
            companded = ALawCompand(input[i] + input[i + 1]);
            output[j] = companded;

            i += 2;
            ++j;
        }
    }
    else if (!input_stereo && output_stereo)
    {
        uint8_t companded = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < input_len && j < output_len)
        {
            companded = ALawCompand(input[i]);
            output[j] = companded;
            output[j + 1] = companded;

            ++i;
            j += 2;
        }
    }
}

void AudioCodec::ALawExpand(const uint8_t* input, const size_t input_len,
    uint16_t* output, const size_t output_len, const bool input_stereo, const bool output_stereo)
{
    if ((input_stereo && output_stereo) || (!input_stereo && !output_stereo))
    {
        const size_t min_len = input_len < output_len ? input_len : output_len;
        for (size_t i = 0; i < min_len; ++i)
        {
            output[i] = ALawExpand(input[i]);
        }
    }
    else if (input_stereo && !output_stereo)
    {
        uint16_t expanded = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < input_len && j < output_len)
        {
            expanded = ALawExpand(input[i]) + ALawExpand(input[i + 1]);
            output[j] = expanded;

            i += 2;
            ++j;
        }
    }
    else if (!input_stereo && output_stereo)
    {
        uint16_t expanded = 0;
        size_t i = 0;
        size_t j = 0;
        while (i < input_len && j < output_len)
        {
            expanded = ALawExpand(input[i]);
            output[j] = expanded;
            output[j + 1] = expanded;

            ++i;
            j += 2;
        }
    }
}

void AudioCodec::ALawCompand(const uint16_t* input, uint8_t* output, const size_t len)
{
    for (size_t i = 0 ; i < len; ++i)
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
    // https://www.cs.columbia.edu/~hgs/research/projects/NetworkAudioLibrary/nal_spring/

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
        sample = -(sample + 1);
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
        mantissa = (sample >> (exponent + 3)) & 0x0F;
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
    // https://www.cs.columbia.edu/~hgs/research/projects/NetworkAudioLibrary/nal_spring/

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
        mantissa = (mantissa + 0x0001) << 3;
    }
    else
    {
        mantissa = (mantissa + 0x0021) << (exponent + 2);
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
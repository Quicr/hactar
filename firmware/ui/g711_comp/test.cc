
#include <stdint.h>
#include <math.h>

#include <iostream>
#include <stdio.h>

const int G711_SAMPLES_PER_FRAME = 160;
const int TABLE_SIZE = 8;
const int BIAS = 0x84;		/* Bias for linear code. */
const int CLIP = 8159;
const int SIGN_BIT = 0x80;	/* Sign bit for a A-law byte. */
const int QUANT_MASK = 0xf;  /* Quantization field mask. */
const int NSEGS = 8;         /* Number of A-law segments. */
const int SEG_SHIFT = 4;     /* Left shift for segment number. */
const int SEG_MASK = 0x70;   /* Segment field mask. */

short seg_aend[TABLE_SIZE] = { 0x1F, 0x3F, 0x7F, 0xFF,
    0x1FF, 0x3FF, 0x7FF, 0xFFF };

short seg_uend[TABLE_SIZE] = { 0x3F, 0x7F, 0xFF, 0x1FF,
    0x3FF, 0x7FF, 0xFFF, 0x1FFF };


inline uint8_t ALawCompand(const uint16_t u_sample)
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
        sample = -(sample+1);
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

inline uint16_t ALawExpand(uint8_t sample)
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


void ALawCompand(const uint16_t* input, uint8_t* output, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        output[i] = ALawCompand(input[i]);
    }
}

void ALawExpand(const uint8_t* input, uint16_t* output, const size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        output[i] = ALawExpand(input[i]);
    }
}


//serach for value in table.
//returns location or TABLE_SIZE if not found
static short search(short val, short* table)
{
    short i;

    for (i = 0; i < TABLE_SIZE; i++)
    {
        if (val <= table[i])
            return (i);
    }

    return (TABLE_SIZE);
}
static short searchAEnd(short val)
{
    return search(val, seg_aend);
}

static short searchUEnd(short val)
{
    return search(val, seg_uend);
}


unsigned char linear2alaw(short pcm_val)
{
    short	 mask;
    short	 seg;
    unsigned char aval;

    pcm_val = pcm_val >> 3;

    if (pcm_val >= 0)
    {
        mask = 0xD5;		/* sign (7th) bit = 1 */
    }
    else
    {
        mask = 0x55;		/* sign bit = 0 */
        pcm_val = -pcm_val - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = searchAEnd(pcm_val);

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8)		/* out of range, return maximum value. */
        return (unsigned char)(0x7F ^ mask);
    else
    {
        aval = (unsigned char)seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 1) & QUANT_MASK;
        else
            aval |= (pcm_val >> seg) & QUANT_MASK;
        return (aval ^ mask);
    }
}

short alaw2linear(unsigned char	a_val)
{
    short t;
    short seg;

    a_val ^= 0x55;

    t = (a_val & QUANT_MASK) << 4;
    seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch (seg)
    {
        case 0:
            t += 8;
            break;
        case 1:
            t += 0x108;
            break;
        default:
            t += 0x108;
            t <<= seg - 1;
    }
    return ((a_val & SIGN_BIT) ? t : -t);
}


int main()
{

    for (int i = 0; i < 0xFFFF; ++i)
    {
        uint8_t in1 = linear2alaw(i);
        uint16_t out1 = alaw2linear(in1);

        uint8_t in2 = ALawCompand(i);
        uint16_t out2 = ALawExpand(in2);

        if (in1 != in2)
        {
            printf("compand mismatch i=%d theirs %u mine %u\n", i, (uint16_t)in1, (uint16_t)in2);
            return 1;
        }
        if (out1 != out2)
        {
            printf("expand mismatch i=%d theirs %u mine %u\n", i, out1, out2);
            return 1;
        }
    }

    printf("Worked percently!\n");

    return 0;
}